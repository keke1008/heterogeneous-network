import { NodeId } from "./nodeId";
import { Cost } from "./cost";
import { ObjectMap, ObjectSet } from "@core/object";
import { match } from "ts-pattern";

export type NetworkUpdate =
    | { type: "NodeUpdated"; id: NodeId; cost: Cost }
    | { type: "NodeRemoved"; id: NodeId }
    | {
          type: "LinkUpdated";
          sourceId: NodeId;
          sourceCost: Cost;
          destinationId: NodeId;
          destinationCost: Cost;
          linkCost: Cost;
      }
    | { type: "LinkRemoved"; sourceId: NodeId; destinationId: NodeId };

type Updated = boolean;

class Links {
    #links = new ObjectMap<NodeId, Cost, string>((id) => id.toString());

    hasLink(id: NodeId): boolean {
        return this.#links.has(id);
    }

    updateLink(id: NodeId, cost: Cost): Updated {
        const current = this.#links.get(id);
        if (current?.equals(cost)) {
            return false;
        } else {
            this.#links.set(id, cost);
            return true;
        }
    }

    removeLink(id: NodeId): Updated {
        return this.#links.delete(id);
    }

    getLinks(): IterableIterator<NodeId> {
        return this.#links.keys();
    }

    getLinksWithCost(): IterableIterator<[NodeId, Cost]> {
        return this.#links.entries();
    }
}

class NodeLinks {
    #strong = new Links();
    #weak = new Links();

    hasLink(id: NodeId): boolean {
        return this.#strong.hasLink(id) || this.#weak.hasLink(id);
    }

    updateStrongLink(id: NodeId, cost: Cost): Updated {
        return this.#strong.updateLink(id, cost);
    }

    updateWeakLink(id: NodeId, cost: Cost): Updated {
        return this.#weak.updateLink(id, cost);
    }

    removeLink(id: NodeId): Updated {
        return this.#strong.removeLink(id) || this.#weak.removeLink(id);
    }

    getAllLinks(): NodeId[] {
        return [...this.#strong.getLinks(), ...this.#weak.getLinks()];
    }

    getStrongLinks(): [NodeId, Cost][] {
        return [...this.#strong.getLinksWithCost()];
    }
}

class NetworkNode {
    readonly #id: NodeId;
    #cost: Cost;
    #links = new NodeLinks();

    private constructor(id: NodeId, cost: Cost) {
        this.#id = id;
        this.#cost = cost;
    }

    static create(id: NodeId, cost: Cost): [NetworkUpdate[], NetworkNode] {
        const node = new NetworkNode(id, cost);
        return [[{ type: "NodeUpdated", id, cost }], node];
    }

    id(): NodeId {
        return this.#id;
    }

    cost(): Cost {
        return this.#cost;
    }

    updateCost(cost: Cost): NetworkUpdate[] {
        const updated = this.#cost.equals(cost);
        this.#cost = cost;
        return updated ? [] : [{ type: "NodeUpdated", id: this.#id, cost }];
    }

    updateStrongLink(destination: NetworkNode, linkCost: Cost): NetworkUpdate[] {
        const updated = this.#links.updateStrongLink(destination.#id, linkCost);
        return updated
            ? [
                  {
                      type: "LinkUpdated",
                      sourceId: this.#id,
                      sourceCost: this.#cost,
                      destinationId: destination.#id,
                      destinationCost: destination.#cost,
                      linkCost,
                  },
              ]
            : [];
    }

    updateWeakLink(destination: NetworkNode, linkCost: Cost): void {
        this.#links.updateWeakLink(destination.#id, linkCost);
    }

    removeLink(id: NodeId): NetworkUpdate[] {
        const updated = this.#links.removeLink(id);
        return updated ? [{ type: "LinkRemoved", sourceId: this.#id, destinationId: id }] : [];
    }

    getAllLinks(): NodeId[] {
        return this.#links.getAllLinks();
    }

    getStrongLinksWithCost(): [NodeId, Cost][] {
        return this.#links.getStrongLinks();
    }
}

class NodePriority {
    static #nextPriority = 0;

    #priorities = new ObjectMap<NodeId, number, string>((id) => id.toString());

    #getOrInsert(id: NodeId): number {
        const priority = this.#priorities.get(id);
        if (priority === undefined) {
            const newPriority = NodePriority.#nextPriority++;
            this.#priorities.set(id, newPriority);
            return newPriority;
        } else {
            return priority;
        }
    }

    prioritize(node1: NetworkNode, node2: NetworkNode): { stronger: NetworkNode; weaker: NetworkNode } {
        const p1 = this.#getOrInsert(node1.id());
        const p2 = this.#getOrInsert(node2.id());
        return p1 < p2 ? { stronger: node1, weaker: node2 } : { stronger: node2, weaker: node1 };
    }
}

export class NetworkState {
    #nodes = new ObjectMap<NodeId, NetworkNode, string>((id) => id.toString());
    #priority = new NodePriority();

    #getOrUpdateNode(id: NodeId, cost: Cost): [NetworkUpdate[], NetworkNode] {
        const node = this.#nodes.get(id);
        if (node === undefined) {
            const [updates, newNode] = NetworkNode.create(id, cost);
            this.#nodes.set(id, newNode);
            return [updates, newNode];
        } else {
            return [node.updateCost(cost), node];
        }
    }

    updateNode(id: NodeId, cost: Cost): NetworkUpdate[] {
        return this.#getOrUpdateNode(id, cost)[0];
    }

    updateLink(
        sourceId: NodeId,
        sourceCost: Cost,
        destinationId: NodeId,
        destinationCost: Cost,
        linkCost: Cost,
    ): NetworkUpdate[] {
        const [n1, sourceNode] = this.#getOrUpdateNode(sourceId, sourceCost);
        const [n2, destinationNode] = this.#getOrUpdateNode(destinationId, destinationCost);

        const { stronger, weaker } = this.#priority.prioritize(sourceNode, destinationNode);
        const n3 = stronger.updateStrongLink(weaker, linkCost);
        weaker.updateWeakLink(stronger, linkCost);
        return [...n1, ...n2, ...n3];
    }

    removeLink(sourceId: NodeId, destinationId: NodeId): NetworkUpdate[] {
        const sourceNode = this.#nodes.get(sourceId);
        const destinationNode = this.#nodes.get(destinationId);
        if (sourceNode === undefined || destinationNode === undefined) {
            return [];
        }

        destinationNode.removeLink(sourceId);
        return sourceNode.removeLink(destinationId);
    }

    removeNode(id: NodeId): NetworkUpdate[] {
        const node = this.#nodes.get(id);
        if (node === undefined) {
            return [];
        }

        const n1 = node.getAllLinks().flatMap((neighbor) => this.removeLink(id, neighbor));
        this.#nodes.delete(id);
        return [...n1, { type: "NodeRemoved", id }];
    }

    removeUnreachableNodes(initialId: NodeId): NetworkUpdate[] {
        const unVisited = new ObjectSet<NodeId, string>((id) => id.toString());
        this.#nodes.forEach((node) => unVisited.add(node.id()));
        unVisited.delete(initialId);

        const queue = [initialId];

        while (queue.length > 0) {
            const currentId = queue.shift()!;
            const currentNode = this.#nodes.get(currentId);
            if (currentNode === undefined) {
                continue;
            }

            for (const neighborId of currentNode.getAllLinks()) {
                if (unVisited.has(neighborId)) {
                    unVisited.delete(neighborId);
                    queue.push(neighborId);
                }
            }
        }

        return [...unVisited.keys()].flatMap((id) => this.removeNode(id));
    }

    dumpAsUpdates(): NetworkUpdate[] {
        const nodes = [...this.#nodes.values()].map((node): NetworkUpdate => {
            return {
                type: "NodeUpdated",
                id: node.id(),
                cost: node.cost(),
            } as const;
        });
        const links = [...this.#nodes.values()].flatMap((node) => {
            return node.getStrongLinksWithCost().map(([neighborId, linkCost]): NetworkUpdate => {
                return {
                    type: "LinkUpdated",
                    sourceId: node.id(),
                    sourceCost: node.cost(),
                    destinationId: neighborId,
                    destinationCost: this.#nodes.get(neighborId)!.cost(),
                    linkCost,
                } as const;
            });
        });
        return [...nodes, ...links];
    }

    applyUpdates(updates: NetworkUpdate[]): NetworkUpdate[] {
        return updates.flatMap((update) => {
            return match(update)
                .with({ type: "NodeUpdated" }, ({ id, cost }) => this.updateNode(id, cost))
                .with({ type: "NodeRemoved" }, ({ id }) => this.removeNode(id))
                .with({ type: "LinkUpdated" }, ({ sourceId, sourceCost, destinationId, destinationCost, linkCost }) => {
                    return this.updateLink(sourceId, sourceCost, destinationId, destinationCost, linkCost);
                })
                .with({ type: "LinkRemoved" }, ({ sourceId, destinationId }) => {
                    return this.removeLink(sourceId, destinationId);
                })
                .exhaustive();
        });
    }
}
