import { ObjectMap } from "@core/object";
import { Address } from "@core/net/link";
import { Cost, NodeId } from "@core/net/node";
import { CancelListening, EventBroker } from "@core/event";

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
    #onNeighborAdded = new EventBroker<Readonly<NeighborNode>>();
    #onNeighborRemoved = new EventBroker<NodeId>();

    constructor() {
        this.addNeighbor(NodeId.loopback(), new Cost(0), Address.loopback());
    }

    onNeighborAdded(listener: (neighbor: Readonly<NeighborNode>) => void): CancelListening {
        return this.#onNeighborAdded.listen(listener);
    }

    onNeighborRemoved(listener: (neighborId: NodeId) => void): CancelListening {
        return this.#onNeighborRemoved.listen(listener);
    }

    addNeighbor(id: NodeId, edgeCost: Cost, mediaAddress: Address) {
        const neighbor = this.#neighbors.get(id);
        if (neighbor === undefined) {
            const entry = new NeighborNodeEntry(id, edgeCost);
            entry.addAddressIfNotExists(mediaAddress);
            this.#neighbors.set(id, entry);
            this.#onNeighborAdded.emit(entry);
        } else {
            neighbor.addAddressIfNotExists(mediaAddress);
        }
    }

    removeNeighbor(id: NodeId) {
        if (this.#neighbors.delete(id)) {
            this.#onNeighborRemoved.emit(id);
        }
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
