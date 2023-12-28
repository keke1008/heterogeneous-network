export { NeighborFrame } from "./frame";

import { Frame, LinkSendError, LinkSendErrorType, LinkSocket } from "@core/net/link";
import { NeighborService } from "../service";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { NodeId } from "@core/net/node";
import { Err, Result } from "oxide.ts";
import { LocalNodeService } from "@core/net/local";
import { NeighborFrame } from "./frame";
import { SingleListenerEventBroker } from "@core/event";
import { sleep } from "@core/async";

export const NeighborSendErrorType = {
    ...LinkSendErrorType,
    Unreachable: "unreachable",
} as const;
export type NeighborSendErrorType = (typeof NeighborSendErrorType)[keyof typeof NeighborSendErrorType];
export type NeighborSendError = LinkSendError | { type: typeof NeighborSendErrorType.Unreachable };

export class NeighborSocket {
    #linkSocket: LinkSocket;
    #localNodeService: LocalNodeService;
    #neighborService: NeighborService;
    #onReceive = new SingleListenerEventBroker<NeighborFrame>();

    constructor(args: {
        linkSocket: LinkSocket;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
    }) {
        this.#linkSocket = args.linkSocket;
        this.#localNodeService = args.localNodeService;
        this.#neighborService = args.neighborService;
        this.#linkSocket.onReceive((frame) => this.#onFrameReceived(frame));
    }

    onReceive(onReceive: (frame: NeighborFrame) => void) {
        this.#onReceive.listen(onReceive);
    }

    async #onFrameReceived(linkFrame: Frame): Promise<void> {
        const result = NeighborFrame.deserialize(linkFrame.reader);
        if (result.isErr()) {
            console.warn(`NeighborSocket: failed to deserialize frame with error: ${result.unwrapErr()}`);
            return;
        }

        const frame = result.unwrap();
        const neighbor = this.#neighborService.getNeighbor(frame.sender.nodeId);
        if (neighbor === undefined) {
            console.warn(`NeighborSocket: received frame from unknown neighbor ${frame.sender.display()}`);
            return;
        }

        const delayCost = neighbor.edgeCost.add(await this.#localNodeService.getCost());
        await sleep(delayCost.intoDuration());

        this.#onReceive.emit(frame);
    }

    async #serializeFrame(payload: BufferReader): Promise<BufferReader> {
        const frame = new NeighborFrame({ sender: await this.#localNodeService.getSource(), reader: payload });
        const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
        frame.serialize(writer);
        return new BufferReader(writer.unwrapBuffer());
    }

    async send(destination: NodeId, payload: BufferReader): Promise<Result<void, NeighborSendError>> {
        const address = await this.#neighborService.resolveAddress(destination);
        if (address.length === 0) {
            return Err({ type: NeighborSendErrorType.Unreachable });
        }

        const frame = new NeighborFrame({ sender: await this.#localNodeService.getSource(), reader: payload });
        return this.#linkSocket.send(address[0], await this.#serializeFrame(frame.reader));
    }

    async sendBroadcast(
        payload: BufferReader,
        opts: { ignoreNodeId?: NodeId; includeLoopback?: boolean } = {},
    ): Promise<void> {
        opts = { includeLoopback: false, ...opts };

        const reader = await this.#serializeFrame(payload);

        const addressType = this.#linkSocket.supportedAddressTypes();
        const reachedAddressType = addressType.filter((type) => {
            return this.#linkSocket.sendBroadcast(type, reader.initialized()).isOk();
        });

        const reached = new Set(reachedAddressType);
        let neighbors = this.#neighborService.getNeighbors();
        if (opts.ignoreNodeId !== undefined) {
            const ignoreNeighbor = opts.ignoreNodeId;
            neighbors = neighbors.filter(({ neighbor }) => !neighbor.nodeId.equals(ignoreNeighbor));
        }
        const notReachedAddress = neighbors
            .map((neighbor) => neighbor.addresses.find((addr) => !reached.has(addr.type())))
            .filter((addr): addr is Exclude<typeof addr, undefined> => addr !== undefined);

        for (const addr of notReachedAddress) {
            if (!opts.includeLoopback && addr.isLoopback()) {
                continue;
            }

            this.#linkSocket.send(addr, reader.initialized());
        }
    }
}
