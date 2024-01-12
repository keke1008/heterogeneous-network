import { ObjectMap } from "@core/object";
import { Address, AddressType } from "@core/net/link";
import { Cost, NodeId, Source } from "@core/net/node";
import { CancelListening, EventBroker } from "@core/event";
import { Delay } from "@core/async";
import { NEIGHBOR_EXPIRATION_TIMEOUT, SEND_HELLO_ITERVAL } from "./constants";
import { NodeInfo } from "../local";

class NeighborNodeTimer {
    expirataion = new Delay(NEIGHBOR_EXPIRATION_TIMEOUT);
    sendHello = new Delay(SEND_HELLO_ITERVAL);

    cancel() {
        this.expirataion.cancel();
        this.sendHello.cancel();
    }
}

class NeighborNodeEntry {
    neighbor: Source;
    edgeCost: Cost;
    addresses: Address[] = [];
    timer? = new NeighborNodeTimer();

    constructor(source: Source, edgeCost: Cost) {
        this.neighbor = source;
        this.edgeCost = edgeCost;
    }

    addAddressIfNotExists(media: Address) {
        if (!this.addresses.some((m) => m.equals(media))) {
            this.addresses.push(media);
        }
    }

    hasAddressType(type: AddressType): boolean {
        return this.addresses.some((addr) => addr.type() === type);
    }

    removeTimer() {
        this.timer?.cancel();
        this.timer = undefined;
    }
}

export interface NeighborNode {
    neighbor: Source;
    edgeCost: Cost;
    addresses: readonly Address[];
}

export class NeighborTable {
    #neighbors = new ObjectMap<NodeId, NeighborNodeEntry>();
    #onNeighborAdded = new EventBroker<Readonly<NeighborNode>>();
    #onNeighborUpdated = new EventBroker<Readonly<NeighborNode>>();
    #onNeighborRemoved = new EventBroker<NodeId>();
    #onHelloInterval = new EventBroker<Readonly<NeighborNode>>();

    constructor() {
        this.addNeighbor(Source.loopback(), new Cost(0), Address.loopback());
        this.#neighbors.get(NodeId.loopback())!.removeTimer();
    }

    initializeLocalNode(info: NodeInfo) {
        this.#neighbors.get(info.id)?.timer!.cancel();
        this.#neighbors.delete(info.id);

        this.addNeighbor(info.source, new Cost(0), Address.loopback());
        this.#neighbors.get(info.id)!.removeTimer();
    }

    onNeighborAdded(listener: (neighbor: Readonly<NeighborNode>) => void): CancelListening {
        return this.#onNeighborAdded.listen(listener);
    }

    onNeighborUpdated(listener: (neighbor: Readonly<NeighborNode>) => void): CancelListening {
        return this.#onNeighborUpdated.listen(listener);
    }

    onNeighborRemoved(listener: (neighborId: NodeId) => void): CancelListening {
        return this.#onNeighborRemoved.listen(listener);
    }

    onHelloInterval(listener: (neighbor: Readonly<NeighborNode>) => void): CancelListening {
        return this.#onHelloInterval.listen(listener);
    }

    addNeighbor(neighbor: Source, edgeCost: Cost, mediaAddress: Address) {
        const neighborEntry = this.#neighbors.get(neighbor.nodeId);
        if (neighborEntry !== undefined) {
            neighborEntry.addAddressIfNotExists(mediaAddress);
            if (!neighborEntry.edgeCost.equals(edgeCost)) {
                neighborEntry.edgeCost = edgeCost;
                this.#onNeighborUpdated.emit(neighborEntry);
            }

            return;
        }

        const entry = new NeighborNodeEntry(neighbor, edgeCost);
        entry.addAddressIfNotExists(mediaAddress);

        this.#neighbors.set(neighbor.nodeId, entry);
        entry.timer!.sendHello.onTimeout(() => this.#onHelloInterval.emit(entry));
        entry.timer!.expirataion.onTimeout(() => {
            console.debug(`NeighborTable: neighbor ${neighbor.nodeId} expired`);
            if (this.#neighbors.delete(neighbor.nodeId)) {
                this.#onNeighborRemoved.emit(neighbor.nodeId);
            }
        });

        this.#onNeighborAdded.emit(entry);
        this.#onNeighborUpdated.emit(entry);
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

    delayExpiration(id: NodeId) {
        const entry = this.#neighbors.get(id);
        entry?.timer?.expirataion.reset();
        console.debug(`NeighborTable: neighbor ${id} expiration delayed`);
    }

    delayHelloInterval(destination: NodeId | AddressType) {
        if (destination instanceof NodeId) {
            const entry = this.#neighbors.get(destination);
            entry?.timer?.sendHello.reset();
        } else {
            for (const entry of this.#neighbors.values()) {
                if (entry.hasAddressType(destination)) {
                    entry.timer?.sendHello.reset();
                }
            }
        }
    }
}
