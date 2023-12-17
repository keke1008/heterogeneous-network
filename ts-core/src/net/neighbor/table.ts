import { ObjectMap } from "@core/object";
import { Address } from "@core/net/link";
import { Cost, NodeId, Source } from "@core/net/node";
import { CancelListening, EventBroker } from "@core/event";

class NeighborNodeEntry {
    neighbor: Source;
    edgeCost: Cost;
    addresses: Address[] = [];

    constructor(source: Source, edgeCost: Cost) {
        this.neighbor = source;
        this.edgeCost = edgeCost;
    }

    addAddressIfNotExists(media: Address) {
        if (!this.addresses.some((m) => m.equals(media))) {
            this.addresses.push(media);
        }
    }
}

export interface NeighborNode {
    neighbor: Source;
    edgeCost: Cost;
    addresses: readonly Address[];
}

export class NeighborTable {
    #neighbors: ObjectMap<NodeId, NeighborNodeEntry, string> = new ObjectMap((n) => n.toString());
    #onNeighborAdded = new EventBroker<Readonly<NeighborNode>>();
    #onNeighborRemoved = new EventBroker<NodeId>();

    constructor() {
        this.addNeighbor(new Source({ nodeId: NodeId.loopback() }), new Cost(0), Address.loopback());
    }

    onNeighborAdded(listener: (neighbor: Readonly<NeighborNode>) => void): CancelListening {
        return this.#onNeighborAdded.listen(listener);
    }

    onNeighborRemoved(listener: (neighborId: NodeId) => void): CancelListening {
        return this.#onNeighborRemoved.listen(listener);
    }

    addNeighbor(neighbor: Source, edgeCost: Cost, mediaAddress: Address) {
        const neighborEntry = this.#neighbors.get(neighbor.nodeId);
        if (neighborEntry === undefined) {
            const entry = new NeighborNodeEntry(neighbor, edgeCost);
            entry.addAddressIfNotExists(mediaAddress);
            this.#neighbors.set(neighbor.nodeId, entry);
            this.#onNeighborAdded.emit(entry);
        } else {
            neighborEntry.addAddressIfNotExists(mediaAddress);
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
