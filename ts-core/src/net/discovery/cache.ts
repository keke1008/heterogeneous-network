import { ObjectMap } from "@core/object";
import { Cost, NodeId } from "../node";
import { DiscoveryFrame, DiscoveryRequestFrame, DiscoveryResponseFrame, Extra } from "./frame";
import { P, match } from "ts-pattern";

class CacheEntry {
    gatewayId: NodeId;
    cost: Cost;
    extra?: Extra;

    constructor(args: { gatewayId: NodeId; cost: Cost; extra?: Extra }) {
        this.gatewayId = args.gatewayId;
        this.cost = args.cost;
        this.extra = args.extra;
    }

    update(args: { gatewayId: NodeId; cost: Cost; extra?: Extra }) {
        this.gatewayId = args.gatewayId;
        this.cost = args.cost;
        if (args.extra) {
            this.extra = args.extra;
        }
    }
}

export class DiscoveryRequestCache {
    #request = new ObjectMap<NodeId, CacheEntry, string>((id) => id.toString());
    #maxSize: number = 8;

    addCache(frame: DiscoveryFrame, additionalCost: Cost) {
        const update = match(frame)
            .with(P.instanceOf(DiscoveryRequestFrame), (frame) => ({
                gatewayId: frame.commonFields.senderId,
                cost: frame.commonFields.totalCost.add(additionalCost),
            }))
            .with(P.instanceOf(DiscoveryResponseFrame), (frame) => ({
                gatewayId: frame.commonFields.senderId,
                cost: frame.commonFields.totalCost.add(additionalCost),
                extra: frame.extra,
            }))
            .exhaustive();

        const existing = this.#request.get(frame.commonFields.sourceId);
        if (existing) {
            existing.update(update);
        } else {
            this.#request.set(frame.commonFields.sourceId, new CacheEntry(update));
        }

        if (this.#request.size > this.#maxSize) {
            this.#request.delete(this.#request.keys().next().value);
        }
    }

    getCache(destinationId: NodeId): CacheEntry | undefined {
        return this.#request.get(destinationId);
    }
}
