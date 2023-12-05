import { deferred } from "@core/deferred";
import { Cost } from "./cost";
import { NodeId } from "./nodeId";
import { Address } from "../link";

export interface NodeInfo {
    id: NodeId;
    cost: Cost;
}

export class LocalNodeInfo {
    #info = deferred<NodeInfo>();

    onAddressAdded(address: Address) {
        if (this.#info.status === "pending") {
            this.#info.resolve({ id: new NodeId(address), cost: new Cost(0) });
        }
    }

    async getId(): Promise<NodeId> {
        return this.#info.then((info) => info.id);
    }

    async getCost(): Promise<Cost> {
        return this.#info.then((info) => info.cost);
    }

    async getInfo(): Promise<NodeInfo> {
        return this.#info;
    }

    async isLocalNodeLikeId(id: NodeId): Promise<boolean> {
        return id.isLoopback() || this.#info.then((info) => info.id.equals(id));
    }

    async convertLocalNodeIdToLoopback(id: NodeId): Promise<NodeId> {
        const localNodeId = await this.getId();
        return localNodeId.equals(id) ? NodeId.loopback() : id;
    }

    hasValue(): this is { id: NodeId; cost: Cost; info: NodeInfo } {
        return this.#info.status === "fulfilled";
    }

    get id(): NodeId | undefined {
        return this.#info.value?.id;
    }

    get cost(): Cost | undefined {
        return this.#info.value?.cost;
    }

    get info(): NodeInfo | undefined {
        return this.#info.value;
    }
}
