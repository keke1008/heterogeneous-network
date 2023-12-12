import { ObjectMap } from "@core/object";
import { NodeId } from "../../node";
import { RouteDiscoveryFrame } from "./frame";
import { Address } from "@core/net/link";

export class CacheEntry {
    gatewayId: NodeId;
    extra?: Address[];

    constructor(gatewayId: NodeId, extra?: Address[]) {
        this.gatewayId = gatewayId;
        this.extra = extra;
    }

    update(gatewayId: NodeId, extra?: Address[]): void {
        this.gatewayId = gatewayId;
        this.extra ??= extra;
    }
}

export class RoutingCache {
    #entries = new ObjectMap<NodeId, CacheEntry, string>((n) => n.toString());
    #maxEntries: number;

    constructor(opt?: { maxEntries?: number }) {
        this.#maxEntries = opt?.maxEntries ?? 8;
    }

    #add(targetId: NodeId, gatewayId: NodeId, extra?: Address[]): void {
        const entry = this.#entries.get(targetId);
        if (entry !== undefined) {
            entry.update(gatewayId, extra);
            return;
        }

        this.#entries.set(targetId, new CacheEntry(gatewayId, extra));
        if (this.#entries.size > this.#maxEntries) {
            const keys = this.#entries.keys().next();
            this.#entries.delete(keys.value);
        }
    }

    addByFrame(frame: RouteDiscoveryFrame): void {
        const extra = Array.isArray(frame.extra) ? frame.extra : undefined;
        this.#add(frame.targetId, frame.senderId, extra);
    }

    get(targetId: NodeId): CacheEntry | undefined {
        return this.#entries.get(targetId);
    }

    getExtra(targetId: NodeId): Address[] | undefined {
        return this.#entries.get(targetId)?.extra;
    }
}
