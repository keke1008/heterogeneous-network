import { ObjectMap, ObjectSet } from "@core/object";
import { Address, AddressType } from "@core/net/link";
import { Cost, NodeId } from "@core/net/node";
import { CancelListening, EventBroker } from "@core/event";
import { Delay, nextTick } from "@core/async";
import { NEIGHBOR_EXPIRATION_TIMEOUT, SEND_HELLO_ITERVAL } from "./constants";
import { NodeInfo } from "../local";

class NeighborNodeTimer {
    expirataion = new Delay(NEIGHBOR_EXPIRATION_TIMEOUT);
    sendHello = new Delay(SEND_HELLO_ITERVAL);

    constructor() {
        this.expirataion.onTimeout(() => {
            this.sendHello.cancel();
        });
    }

    cancel() {
        this.expirataion.cancel();
        this.sendHello.cancel();
    }
}

export interface NeighborNode {
    neighbor: NodeId;
    edgeCost: Cost;
    addresses: readonly Address[];
}

class NeighborNodeEntry implements NeighborNode {
    neighbor: NodeId;
    edgeCost: Cost;
    addresses: Address[] = [];
    timer? = new NeighborNodeTimer();

    constructor(source: NodeId, edgeCost: Cost) {
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

export class NeighborTable {
    #neighbors = new ObjectMap<NodeId, NeighborNodeEntry>();
    #eventExcludedNeighbors = new ObjectSet<NodeId>();
    #onNeighborAdded = new EventBroker<Readonly<NeighborNode>>();
    #onNeighborUpdated = new EventBroker<Readonly<NeighborNode>>();
    #onNeighborRemoved = new EventBroker<NodeId>();
    #onHelloInterval = new EventBroker<Readonly<NeighborNode>>();

    constructor() {
        this.#eventExcludedNeighbors.add(NodeId.loopback());
        this.addNeighbor(NodeId.loopback(), new Cost(0), Address.loopback());
        this.#neighbors.get(NodeId.loopback())!.removeTimer();
    }

    initializeLocalNode(info: NodeInfo) {
        this.#neighbors.get(info.id)?.timer!.cancel();
        this.#neighbors.delete(info.id);

        this.#eventExcludedNeighbors.add(info.id);
        this.addNeighbor(info.source.nodeId, new Cost(0), Address.loopback());
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

    addNeighbor(neighbor: NodeId, edgeCost: Cost, mediaAddress: Address) {
        const neighborEntry = this.#neighbors.get(neighbor);
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
        this.#neighbors.set(neighbor, entry);

        if (this.#eventExcludedNeighbors.has(neighbor)) {
            return;
        }

        entry.timer!.sendHello.onTimeout(() => this.#onHelloInterval.emit(entry));
        entry.timer!.expirataion.onTimeout(() => {
            if (this.#neighbors.delete(neighbor)) {
                this.#onNeighborRemoved.emit(neighbor);
            }
        });

        // neighborが追加されたことをトリガーとして，新たに追加されたneighborにフレームを送るプログラムがある．
        // 相手にHelloを返信する前にそのようなプログラムが実行されると，
        // 相手にとって自分はNeighborではないと判断され，フレームが捨てられてしまう．
        // それを防ぐために，neighbor追加イベントの通知を次のTickに遅延させる必要がある．
        nextTick(() => {
            this.#onNeighborAdded.emit(entry);
            this.#onNeighborUpdated.emit(entry);
        });
    }

    getAddresses(id: NodeId): Address[] {
        return this.#neighbors.get(id)?.addresses ?? [];
    }

    resolveNeighborFromAddress(address: Address): NeighborNode | undefined {
        for (const entry of this.#neighbors.values()) {
            if (entry.addresses.some((addr) => addr.equals(address))) {
                return entry;
            }
        }

        return undefined;
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
