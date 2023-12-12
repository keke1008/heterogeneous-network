import { NodeId, Cost } from "@core/net";
import { bounds } from "@core/numeric";
import { ObjectMap } from "@core/object";

class RoutingEntryId {
    #id: number;

    constructor(id: number) {
        bounds.assertUint16(id);
        this.#id = id;
    }

    get(): number {
        return this.#id;
    }
}

interface RoutingEntry {
    id: RoutingEntryId;
    destination: NodeId;
    gateway: NodeId;
    cost: Cost;
}

class RoutingTable {
    #nextId: number = 0;
    #entries = new ObjectMap<RoutingEntryId, RoutingEntry, number>((id) => id.get());

    update(entry: RoutingEntry): void {
        const existing = this.#entries.get(entry.id);
        if (existing === undefined) {
            const id = new RoutingEntryId(this.#nextId++);
            this.#entries.set(id, entry);
        } else {
            existing.destination = entry.destination;
            existing.gateway = entry.gateway;
            existing.cost = entry.cost;
        }
    }

    remove(id: RoutingEntryId): void {
        this.#entries.delete(id);
    }

    resolveGatewayNode(destination: NodeId): NodeId | undefined {
        for (const entry of this.#entries.values()) {
            if (entry.destination.equals(destination)) {
                return entry.gateway;
            }
        }
    }
}
