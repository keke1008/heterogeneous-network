import { ObjectMap } from "@core/object";
import { Cost, NodeId } from "../node";
import { DiscoveryFrame, DiscoveryFrameType } from "./frame";
import { unreachable } from "@core/types";

class CacheEntry {
    gatewayId: NodeId;
    cost: Cost;

    constructor(args: { gatewayId: NodeId; cost: Cost }) {
        this.gatewayId = args.gatewayId;
        this.cost = args.cost;
    }

    update(args: { gatewayId: NodeId; cost: Cost }) {
        this.gatewayId = args.gatewayId;
        this.cost = args.cost;
    }
}

export class DiscoveryRequestCache {
    #request = new ObjectMap<NodeId, CacheEntry, string>((id) => id.toString());
    #maxSize: number = 8;

    addCache(frame: DiscoveryFrame, additionalCost: Cost) {
        let update;
        if (frame.type === DiscoveryFrameType.Request) {
            update = { gatewayId: frame.senderId, cost: frame.totalCost.add(additionalCost) };
        } else if (frame.type === DiscoveryFrameType.Response) {
            update = { gatewayId: frame.senderId, cost: frame.totalCost.add(additionalCost) };
        } else {
            return unreachable(frame.type);
        }

        const existing = this.#request.get(frame.sourceId);
        if (existing) {
            existing.update(update);
        } else {
            this.#request.set(frame.sourceId, new CacheEntry(update));
        }

        if (this.#request.size > this.#maxSize) {
            this.#request.delete(this.#request.keys().next().value);
        }
    }

    getCache(destinationId: NodeId): CacheEntry | undefined {
        return this.#request.get(destinationId);
    }
}
