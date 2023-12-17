import { deferred } from "@core/deferred";
import { Cost } from "./cost";
import { NodeId } from "./nodeId";
import { LinkService } from "../link";
import { ClusterId, NoCluster } from "./clusterId";
import { Source } from "./source";

export class NodeInfo {
    #source: Source;
    #cost: Cost;

    constructor(args: { source: Source; cost: Cost }) {
        this.#source = args.source;
        this.#cost = args.cost;
    }

    get id(): NodeId {
        return this.#source.nodeId;
    }

    get cost(): Cost {
        return this.#cost;
    }

    get clusterId(): ClusterId | NoCluster {
        return this.#source.clusterId;
    }

    get source(): Source {
        return this.#source;
    }
}

export class LocalNodeService {
    #info = deferred<NodeInfo>();

    constructor(args: { linkService: LinkService }) {
        args.linkService.onAddressAdded((address) => {
            if (this.#info.status === "pending") {
                this.#info.resolve(
                    new NodeInfo({
                        source: new Source({ nodeId: new NodeId(address) }),
                        cost: new Cost(0),
                    }),
                );
            }
        });
    }

    async getId(): Promise<NodeId> {
        return this.#info.then((info) => info.id);
    }

    async getCost(): Promise<Cost> {
        return this.#info.then((info) => info.cost);
    }

    async getSource(): Promise<Source> {
        return this.#info.then((info) => info.source);
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
            this.#info.resolve(
                new NodeInfo({
                    source: new Source({ nodeId: id, clusterId: clusterId }),
                    cost: cost ?? new Cost(0),
                }),
            );
        }
    }
}
