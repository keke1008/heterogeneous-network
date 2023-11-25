import { Frame, LinkSendError, LinkSendErrorType, LinkSocket } from "@core/net/link";
import { NeighborService } from "./service";
import { BufferReader } from "@core/net/buffer";
import { NodeId } from "../node";
import { Err, Result } from "oxide.ts";

export const NeighborSendErrorType = {
    ...LinkSendErrorType,
    Unreachable: "unreachable",
} as const;
export type NeighborSendErrorType = (typeof NeighborSendErrorType)[keyof typeof NeighborSendErrorType];
export type NeighborSendError = LinkSendError | { type: typeof NeighborSendErrorType.Unreachable };

export class NeighborSocket {
    #linkSocket: LinkSocket;
    #neighborService: NeighborService;

    constructor(args: { linkSocket: LinkSocket; neighborService: NeighborService }) {
        this.#neighborService = args.neighborService;
        this.#linkSocket = args.linkSocket;
    }

    onReceive(onReceive: (frame: Frame) => void) {
        this.#linkSocket.onReceive(onReceive);
    }

    send(destination: NodeId, reader: BufferReader): Result<void, NeighborSendError> {
        const address = this.#neighborService.resolveAddress(destination);
        if (address.length === 0) {
            return Err({ type: NeighborSendErrorType.Unreachable });
        }
        return this.#linkSocket.send(address[0], reader);
    }

    sendBroadcast(reader: BufferReader, ignoreNodeId?: NodeId): void {
        const addressType = this.#linkSocket.supportedAddressTypes();
        const reachedAddressType = addressType.filter((type) => {
            return this.#linkSocket.sendBroadcast(type, reader.initialized()).isOk();
        });

        const reached = new Set(reachedAddressType);
        let neighbors = this.#neighborService.getNeighbors();
        if (ignoreNodeId !== undefined) {
            neighbors = neighbors.filter((neighbor) => !neighbor.id.equals(ignoreNodeId));
        }
        const notReachedAddress = neighbors
            .map((neighbor) => neighbor.addresses.find((addr) => !reached.has(addr.type())))
            .filter((addr): addr is Exclude<typeof addr, undefined> => addr !== undefined);

        for (const addr of notReachedAddress) {
            this.#linkSocket.send(addr, reader.initialized());
        }
    }
}
