import { deferred } from "@core/deferred";
import { Cost } from "./cost";
import { NodeId } from "./nodeId";
import { LinkService } from "../link";
import { ClusterId } from "./clusterId";

export interface NodeInfo {
    id: NodeId;
    cost: Cost;
    clusterId: ClusterId;
}

export class LocalNodeService {
    #info = deferred<NodeInfo>();

    constructor(args: { linkService: LinkService }) {
        args.linkService.onAddressAdded((address) => {
            if (this.#info.status === "pending") {
                this.#info.resolve({
                    id: new NodeId(address),
                    cost: new Cost(0),
                    clusterId: ClusterId.default(),
                });
            }
        });
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

    tryInitialize(id: NodeId, cost?: Cost, clusterId?: ClusterId): void {
        if (this.#info.status === "pending") {
            this.#info.resolve({
                id,
                cost: cost ?? new Cost(0),
                clusterId: clusterId ?? ClusterId.default(),
            });
        }
    }
}
