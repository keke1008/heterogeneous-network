export { ReceivedNeighborFrame as NeighborFrame } from "./frame";

import { Frame, LinkSendError, LinkSendErrorType, LinkSocket } from "@core/net/link";
import { NeighborService } from "../service";
import { NodeId } from "@core/net/node";
import { Err, Ok, Result } from "oxide.ts";
import { LocalNodeService } from "@core/net/local";
import { ReceivedNeighborFrame } from "./frame";
import { SingleListenerEventBroker } from "@core/event";
import { sleep } from "@core/async";

export const NeighborSendErrorType = {
    ...LinkSendErrorType,
    Unreachable: "unreachable",
} as const;
export type NeighborSendErrorType = (typeof NeighborSendErrorType)[keyof typeof NeighborSendErrorType];
export type NeighborSendError = LinkSendError | { type: typeof NeighborSendErrorType.Unreachable };

export interface NeighborSocketConfig {
    doDelay: boolean;
}

export class NeighborSocket {
    #linkSocket: LinkSocket;
    #config: NeighborSocketConfig;
    #localNodeService: LocalNodeService;
    #neighborService: NeighborService;
    #onReceive = new SingleListenerEventBroker<ReceivedNeighborFrame>();

    constructor(args: {
        linkSocket: LinkSocket;
        config: NeighborSocketConfig;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
    }) {
        this.#linkSocket = args.linkSocket;
        this.#config = args.config;
        this.#localNodeService = args.localNodeService;
        this.#neighborService = args.neighborService;
        this.#linkSocket.onReceive((frame) => this.#onFrameReceived(frame));
    }

    onReceive(onReceive: (frame: ReceivedNeighborFrame) => void) {
        this.#onReceive.listen(onReceive);
    }

    async #onFrameReceived(frame: Frame): Promise<void> {
        const neighbor = this.#neighborService.resolveNeighborFromAddress(frame.remote);
        if (neighbor === undefined) {
            console.info("Received frame from unknown neighbor", frame.remote);
            return;
        }

        if (this.#config.doDelay) {
            const delayCost = neighbor.linkCost.add(await this.#localNodeService.getCost());
            await sleep(delayCost.intoDuration());
        }

        this.#neighborService.onFrameReceived(neighbor.neighbor);
        this.#onReceive.emit(new ReceivedNeighborFrame({ sender: neighbor.neighbor, payload: frame.payload }));
    }

    async send(destination: NodeId, payload: Uint8Array): Promise<Result<void, NeighborSendError>> {
        const address = await this.#neighborService.resolveAddress(destination);
        if (address.length === 0) {
            return Err({ type: NeighborSendErrorType.Unreachable });
        }

        const result = await this.#linkSocket.send(address[0], payload);
        if (result.isErr()) {
            return Err(result.unwrapErr());
        }

        this.#neighborService.onFrameSent(destination);
        return Ok(undefined);
    }

    async sendBroadcast(
        payload: Uint8Array,
        opts: { ignoreNodeId?: NodeId; includeLoopback?: boolean } = {},
    ): Promise<void> {
        opts = { includeLoopback: false, ...opts };

        const addressType = this.#linkSocket.supportedAddressTypes();
        const reachedAddressType = addressType.filter((type) => {
            return this.#linkSocket.sendBroadcast(type, payload).isOk();
        });
        reachedAddressType.forEach((type) => this.#neighborService.onFrameSent(type));

        const reached = new Set(reachedAddressType);
        let notReachedNeighbors = this.#neighborService
            .getNeighbors()
            .filter(({ addresses }) => !addresses.some((addr) => reached.has(addr.type())));
        if (opts.ignoreNodeId !== undefined) {
            const ignoreNeighbor = opts.ignoreNodeId;
            notReachedNeighbors = notReachedNeighbors.filter(({ neighbor }) => !neighbor.equals(ignoreNeighbor));
        }

        for (const { neighbor, addresses } of notReachedNeighbors) {
            if (!opts.includeLoopback && neighbor.isLoopback()) {
                continue;
            }

            if (addresses.length === 0) {
                continue;
            }

            const result = await this.#linkSocket.send(addresses[0], payload);
            if (result.isOk()) {
                this.#neighborService.onFrameSent(neighbor);
            }
        }
    }

    maxPayloadLength(): number {
        return this.#linkSocket.maxPayloadLength();
    }
}
