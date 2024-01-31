import { NodeId } from "./nodeId";
import { Cost } from "./cost";
import { ObjectMap, ObjectSet } from "@core/object";
import { match } from "ts-pattern";
import { OptionalClusterId } from "./clusterId";

export interface PartialNode {
    nodeId: NodeId;
    clusterId?: OptionalClusterId;
    cost?: Cost;
}

export type NetworkTopologyUpdate =
    | { type: "NodeUpdated"; node: PartialNode }
    | { type: "NodeRemoved"; id: NodeId }
    | { type: "LinkUpdated"; source: PartialNode; destination: PartialNode; linkCost: Cost }
    | { type: "LinkRemoved"; sourceId: NodeId; destinationId: NodeId };

type Updated = boolean;

class Links {
    #links = new ObjectMap<NodeId, Cost>();

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
    #node: PartialNode;
    #links = new NodeLinks();

    private constructor(node: PartialNode) {
        this.#node = node;
    }

    static create(node: PartialNode): [NetworkTopologyUpdate[], NetworkNode] {
        return [[{ type: "NodeUpdated", node }], new NetworkNode(node)];
    }

    node(): PartialNode {
        return this.#node;
    }

    update(node: PartialNode): NetworkTopologyUpdate[] {
        const costUpdated = node.cost !== undefined && !this.#node.cost?.equals(node.cost);
        costUpdated && (this.#node.cost = node.cost);

        const clusterIdUpdated = node.clusterId !== undefined && !this.#node.clusterId?.equals(node.clusterId);
        clusterIdUpdated && (this.#node.clusterId = node.clusterId);

        return costUpdated || clusterIdUpdated ? [{ type: "NodeUpdated", node }] : [];
    }

    updateStrongLink(destination: NetworkNode, linkCost: Cost): NetworkTopologyUpdate[] {
        const updated = this.#links.updateStrongLink(destination.#node.nodeId, linkCost);
        return updated ? [{ type: "LinkUpdated", source: this.#node, destination: destination.#node, linkCost }] : [];
    }

    updateWeakLink(destination: NetworkNode, linkCost: Cost): void {
        this.#links.updateWeakLink(destination.#node.nodeId, linkCost);
    }

    removeLink(id: NodeId): NetworkTopologyUpdate[] {
        const updated = this.#links.removeLink(id);
        return updated ? [{ type: "LinkRemoved", sourceId: this.#node.nodeId, destinationId: id }] : [];
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

    #priorities = new ObjectMap<NodeId, number>();

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
        const p1 = this.#getOrInsert(node1.node().nodeId);
        const p2 = this.#getOrInsert(node2.node().nodeId);
        return p1 < p2 ? { stronger: node1, weaker: node2 } : { stronger: node2, weaker: node1 };
    }
}

export class NetworkState {
    #nodes = new ObjectMap<NodeId, NetworkNode>();
    #priority = new NodePriority();

    #getOrUpdateNode(node: PartialNode): [NetworkTopologyUpdate[], NetworkNode] {
        const exists = this.#nodes.get(node.nodeId);
        if (exists === undefined) {
            const [updates, newNode] = NetworkNode.create(node);
            this.#nodes.set(node.nodeId, newNode);
            return [updates, newNode];
        } else {
            return [exists.update(node), exists];
        }
    }

    updateNode(node: PartialNode): NetworkTopologyUpdate[] {
        return this.#getOrUpdateNode(node)[0];
    }

    updateLink(source: PartialNode, destination: PartialNode, linkCost: Cost): NetworkTopologyUpdate[] {
        const [n1, sourceNode] = this.#getOrUpdateNode(source);
        const [n2, destinationNode] = this.#getOrUpdateNode(destination);

        const { stronger, weaker } = this.#priority.prioritize(sourceNode, destinationNode);
        const n3 = stronger.updateStrongLink(weaker, linkCost);
        weaker.updateWeakLink(stronger, linkCost);
        return [...n1, ...n2, ...n3];
    }

    removeLink(sourceId: NodeId, destinationId: NodeId): NetworkTopologyUpdate[] {
        const sourceNode = this.#nodes.get(sourceId);
        const destinationNode = this.#nodes.get(destinationId);
        if (sourceNode === undefined || destinationNode === undefined) {
            return [];
        }

        destinationNode.removeLink(sourceId);
        return sourceNode.removeLink(destinationId);
    }

    syncLink(sourceId: NodeId, links: { nodeId: NodeId; linkCost: Cost }[]) {
        const [n1, sourceNode] = this.#getOrUpdateNode({ nodeId: sourceId });
        const n2 = links.flatMap(({ nodeId, linkCost }) => {
            const [n1, destinationNode] = this.#getOrUpdateNode({ nodeId });
            const { stronger, weaker } = this.#priority.prioritize(sourceNode, destinationNode);
            const n2 = stronger.updateStrongLink(weaker, linkCost);
            weaker.updateWeakLink(stronger, linkCost);
            return [...n1, ...n2];
        });

        const sourceLinks = new ObjectSet<NodeId>();
        links.forEach(({ nodeId }) => sourceLinks.add(nodeId));
        const n3 = sourceNode.getAllLinks().flatMap((neighborId) => {
            return sourceLinks.has(neighborId) ? [] : this.removeLink(sourceId, neighborId);
        });

        return [...n1, ...n2, ...n3];
    }

    removeNode(id: NodeId): NetworkTopologyUpdate[] {
        const node = this.#nodes.get(id);
        if (node === undefined) {
            return [];
        }

        const n1 = node.getAllLinks().flatMap((neighbor) => this.removeLink(id, neighbor));
        this.#nodes.delete(id);
        return [...n1, { type: "NodeRemoved", id }];
    }

    getUnreachableNodes(initialId: NodeId): ObjectSet<NodeId> {
        const unVisited = new ObjectSet<NodeId>();
        this.#nodes.forEach((node) => unVisited.add(node.node().nodeId));
        unVisited.delete(initialId);

        const queue = [initialId];
        while (queue.length > 0) {
            const currentId = queue.shift()!;
            const currentNode = this.#nodes.get(currentId);
            if (currentNode === undefined) {
                continue;
            }

            const unVisitedNeighbors = currentNode.getAllLinks().filter((neighborId) => unVisited.has(neighborId));
            for (const neighborId of unVisitedNeighbors) {
                unVisited.delete(neighborId);
                queue.push(neighborId);
            }
        }

        return unVisited;
    }

    dumpAsUpdates(): NetworkTopologyUpdate[] {
        const nodes = [...this.#nodes.values()].map((node): NetworkTopologyUpdate => {
            return { type: "NodeUpdated", node: node.node() } as const;
        });
        const links = [...this.#nodes.values()].flatMap((node) => {
            return node.getStrongLinksWithCost().map(([neighborId, linkCost]): NetworkTopologyUpdate => {
                const neighbor = this.#nodes.get(neighborId);
                if (neighbor === undefined) {
                    throw new Error("Invalid state: neighbor not found");
                }
                return { type: "LinkUpdated", source: node.node(), destination: neighbor.node(), linkCost };
            });
        });
        return [...nodes, ...links];
    }

    applyUpdates(updates: NetworkTopologyUpdate[]): NetworkTopologyUpdate[] {
        return updates.flatMap((update) => {
            return match(update)
                .with({ type: "NodeUpdated" }, ({ node }) => this.updateNode(node))
                .with({ type: "NodeRemoved" }, ({ id }) => this.removeNode(id))
                .with({ type: "LinkUpdated" }, ({ source, destination, linkCost }) => {
                    return this.updateLink(source, destination, linkCost);
                })
                .with({ type: "LinkRemoved" }, ({ sourceId, destinationId }) => {
                    return this.removeLink(sourceId, destinationId);
                })
                .exhaustive();
        });
    }
}
