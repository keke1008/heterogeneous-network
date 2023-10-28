import { Frame, LinkSocket } from "@core/net/link";
import { NeighborService } from "./service";
import { BufferReader } from "@core/net/buffer";
import { NodeId } from "../node";

export type SendResult = { result: "success" } | { result: "failure"; reason: "unreachable" };

export class NeighborSocket {
    #linkSocket: LinkSocket;
    #neighborService: NeighborService;
    #onReceive?: (frame: Frame) => void;

    constructor(args: { linkSocket: LinkSocket; neighborService: NeighborService }) {
        this.#neighborService = args.neighborService;
        this.#linkSocket = args.linkSocket;
    }

    onReceive(onReceive: (frame: Frame) => void) {
        if (this.#onReceive) {
            throw new Error("onReceive already set");
        }
        this.#onReceive = onReceive;
    }

    send(destination: NodeId, reader: BufferReader): SendResult {
        const address = this.#neighborService.resolveAddress(destination);
        if (address.length === 0) {
            return { result: "failure", reason: "unreachable" };
        }

        this.#linkSocket.send(address[0], reader);
        return { result: "success" };
    }

    sendBroadcast(reader: BufferReader, ignoreNodeId?: NodeId): void {
        const addressType = this.#linkSocket.supportedAddressTypes();
        const reachedAddressType = addressType.filter((type) => {
            const result = this.#linkSocket.sendBroadcast(type, reader.initialized());
            return result === "success";
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
