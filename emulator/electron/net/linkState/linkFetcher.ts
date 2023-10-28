import { NodeId, RpcService, RpcStatus } from "@core/net";
import { ObjectSet } from "@core/object";

export class LinkFetcher {
    #queue: ObjectSet<NodeId, string> = new ObjectSet((id) => id.toString());
    #rpc: RpcService;
    #onReceive?: (nodeId: NodeId, linkIds: NodeId[]) => void;
    #worker: Promise<void> = Promise.resolve();

    constructor(rpc: RpcService) {
        this.#rpc = rpc;
    }

    onReceive(onReceived: (nodeId: NodeId, linkIds: NodeId[]) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceived is already set");
        }
        this.#onReceive = onReceived;
    }

    async #fetch(nodeId: NodeId): Promise<void> {
        const result = await this.#rpc.requestRoutingNeighborGetNeighborList(nodeId);
        if (result.status === RpcStatus.Success) {
            this.#onReceive?.(nodeId, result.value);
        }
    }

    requestFetch(nodeId: NodeId): void {
        if (this.#queue.has(nodeId)) {
            return;
        }
        this.#queue.add(nodeId);
        this.#worker = this.#worker.then(() => this.#fetch(nodeId)).finally(() => this.#queue.delete(nodeId));
    }
}
