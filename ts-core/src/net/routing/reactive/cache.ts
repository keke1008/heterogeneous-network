import { NodeId } from "../node";

class CacheEntry {
    targetId: NodeId;
    gatewayId: NodeId;

    constructor(targetId: NodeId, gatewayId: NodeId) {
        this.targetId = targetId;
        this.gatewayId = gatewayId;
    }
}

export class RoutingCache {
    #entries: CacheEntry[];
    #maxEntries: number;

    constructor(opt?: { maxEntries?: number }) {
        this.#entries = [];
        this.#maxEntries = opt?.maxEntries ?? 8;
    }

    add(targetId: NodeId, gatewayId: NodeId): void {
        this.#entries.push(new CacheEntry(targetId, gatewayId));
        if (this.#entries.length > this.#maxEntries) {
            this.#entries.shift();
        }
    }

    remove(gatewayId: NodeId): void {
        this.#entries = this.#entries.filter((entry) => !entry.gatewayId.equals(gatewayId));
    }

    get(targetId: NodeId): NodeId | undefined {
        for (const entry of this.#entries) {
            if (entry.targetId.equals(targetId)) {
                return entry.gatewayId;
            }
        }
        return undefined;
    }
}
