import { ObjectMap } from "@core/object";
import { Address } from "@core/net/link";
import { Cost, NodeId } from "@core/net/node";

class NeighborNodeEntry {
    id: NodeId;
    edgeCost: Cost;
    addresses: Address[] = [];

    constructor(id: NodeId, edgeCost: Cost) {
        this.id = id;
        this.edgeCost = edgeCost;
    }

    addAddressIfNotExists(media: Address) {
        if (!this.addresses.some((m) => m.equals(media))) {
            this.addresses.push(media);
        }
    }
}

export interface NeighborNode {
    id: NodeId;
    edgeCost: Cost;
    addresses: readonly Address[];
}

export class NeighborTable {
    #neighbors: ObjectMap<NodeId, NeighborNodeEntry, string> = new ObjectMap((n) => n.toString());

    addNeighbor(id: NodeId, edgeCost: Cost, media: Address) {
        this.#neighbors.has(id) || this.#neighbors.set(id, new NeighborNodeEntry(id, edgeCost));
        this.#neighbors.get(id)!.addAddressIfNotExists(media);
    }

    removeNeighbor(id: NodeId) {
        this.#neighbors.delete(id);
    }

    getAddresses(id: NodeId): Address[] {
        return this.#neighbors.get(id)?.addresses ?? [];
    }

    getCost(id: NodeId): Cost | undefined {
        return this.#neighbors.get(id)?.edgeCost;
    }

    getNeighbor(id: NodeId): NeighborNode | undefined {
        return this.#neighbors.get(id);
    }

    getNeighbors(): NeighborNode[] {
        return [...this.#neighbors.values()];
    }
}
