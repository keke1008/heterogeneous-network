import { ObjectMap, ObjectSet } from "@core/object";
import { Cost, NodeId } from "@core/net/node";
import { StateUpdate } from "./update";

class NodeLinks {
    #strong: ObjectMap<NodeId, Cost, string> = new ObjectMap((id) => id.toString());
    #weak: ObjectMap<NodeId, Cost, string> = new ObjectMap((id) => id.toString());

    hasLink(nodeId: NodeId): boolean {
        return this.#strong.has(nodeId) || this.#weak.has(nodeId);
    }

    #linkAddable(nodeId: NodeId): boolean {
        return !this.hasLink(nodeId);
    }

    addStrongLink(nodeId: NodeId, cost: Cost): "added" | "costUpdated" | "alreadyExists" {
        if (this.#linkAddable(nodeId)) {
            this.#strong.set(nodeId, cost);
            return "added";
        } else if (this.#strong.get(nodeId)?.equals(cost) !== true) {
            this.#strong.set(nodeId, cost);
            return "costUpdated";
        } else {
            return "alreadyExists";
        }
    }

    addWeakLink(nodeId: NodeId, cost: Cost): void {
        if (this.#linkAddable(nodeId)) {
            this.#weak.set(nodeId, cost);
        }
    }

    removeLink(nodeId: NodeId): "removed" | "notExists" {
        if (this.#strong.delete(nodeId)) {
            return "removed";
        } else {
            this.#weak.delete(nodeId);
            return "notExists";
        }
    }

    getLinks(): NodeId[] {
        return [...this.#strong.keys(), ...this.#weak.keys()];
    }

    getStrongLinks(): [NodeId, Cost][] {
        return [...this.#strong.entries()];
    }
}

class NetworkNode {
    readonly nodeId: NodeId;
    #cost?: Cost;
    #links: NodeLinks | undefined;

    constructor(nodeId: NodeId, cost?: Cost) {
        this.nodeId = nodeId;
        this.#cost = cost;
    }

    getCost(): Cost | undefined {
        return this.#cost;
    }

    updateCost(cost: Cost): StateUpdate {
        if (this.#cost?.equals(cost) !== true) {
            this.#cost = cost;
            return new StateUpdate({
                nodeCostChanged: [{ nodeId: this.nodeId, cost }],
            });
        }
        return new StateUpdate();
    }

    addStrongLink(nodeId: NodeId, linkCost: Cost): StateUpdate {
        this.#links ??= new NodeLinks();
        const result = this.#links.addStrongLink(nodeId, linkCost);
        switch (result) {
            case "added":
                return new StateUpdate({
                    linkAdded: [{ nodeId1: this.nodeId, nodeId2: nodeId, cost: linkCost }],
                });
            case "costUpdated":
                return new StateUpdate({
                    linkCostChanged: [{ nodeId1: this.nodeId, nodeId2: nodeId, cost: linkCost }],
                });
            case "alreadyExists":
                return new StateUpdate();
        }
    }

    addWeakLink(nodeId: NodeId, linkCost: Cost): void {
        this.#links ??= new NodeLinks();
        this.#links.addWeakLink(nodeId, linkCost);
    }

    removeLink(nodeId: NodeId): StateUpdate {
        const result = this.#links?.removeLink(nodeId);
        if (result === "removed") {
            return new StateUpdate({
                linkRemoved: [{ nodeId1: this.nodeId, nodeId2: nodeId }],
            });
        } else {
            return new StateUpdate();
        }
    }

    getLinks(): NodeId[] | undefined {
        return this.#links?.getLinks();
    }

    getStrongLinks(): [NodeId, Cost][] | undefined {
        return this.#links?.getStrongLinks();
    }
}

export class LinkState {
    #nodes: ObjectMap<NodeId, NetworkNode, string> = new ObjectMap((id) => id.toString());
    #localId: NodeId;

    private constructor(localId: NodeId, localCost?: Cost) {
        this.#localId = localId;
        this.createOrUpdateNode(localId, localCost);
    }

    static create(localId: NodeId, localCost?: Cost): [LinkState, StateUpdate] {
        const state = new LinkState(localId, localCost);
        return [state, state.createOrUpdateNode(localId, localCost)];
    }

    #getOrCreateOrUpdateNode(nodeId: NodeId, cost?: Cost): [NetworkNode, StateUpdate] {
        const node = this.#nodes.get(nodeId);
        if (node === undefined) {
            const node = new NetworkNode(nodeId, cost);
            this.#nodes.set(node.nodeId, node);
            return [node, new StateUpdate({ nodeAdded: [{ nodeId, cost }] })];
        } else if (cost !== undefined) {
            return [node, node.updateCost(cost)];
        } else {
            return [node, new StateUpdate()];
        }
    }

    createOrUpdateNode(nodeId: NodeId, cost?: Cost): StateUpdate {
        return this.#getOrCreateOrUpdateNode(nodeId, cost)[1];
    }

    createOrUpdateLink(sourceId: NodeId, targetId: NodeId, linkCost: Cost): StateUpdate {
        const [source, result1] = this.#getOrCreateOrUpdateNode(sourceId);
        const [target, result2] = this.#getOrCreateOrUpdateNode(targetId);
        const result3 = source.addStrongLink(target.nodeId, linkCost);
        target.addWeakLink(source.nodeId, linkCost);
        return StateUpdate.merge(result1, result2, result3);
    }

    #removeLink(sourceId: NodeId, targetId: NodeId): StateUpdate {
        const source = this.#nodes.get(sourceId);
        const target = this.#nodes.get(targetId);
        if (source === undefined || target === undefined) {
            return new StateUpdate();
        } else {
            return StateUpdate.merge(source.removeLink(targetId), target.removeLink(sourceId));
        }
    }

    #removeNode(nodeId: NodeId): StateUpdate {
        const node = this.#nodes.get(nodeId);
        if (node === undefined) {
            return new StateUpdate();
        } else {
            const results = (node.getLinks() ?? [])
                .flatMap((id) => this.#nodes.get(id) ?? [])
                .map((node) => this.#removeLink(nodeId, node.nodeId));

            this.#nodes.delete(nodeId);
            const result = new StateUpdate({ nodeRemoved: [nodeId] });
            return StateUpdate.merge(...results, result);
        }
    }

    #removeIsolatedNodes(): StateUpdate {
        const visited = new ObjectSet<NodeId, string>((id) => id.toString());
        const queue: NodeId[] = [this.#localId];

        while (queue.length > 0) {
            const id = queue.shift()!;
            visited.add(id);

            const node = this.#nodes.get(id)!;
            for (const link of node.getLinks() ?? []) {
                if (!visited.has(link)) {
                    queue.push(link);
                }
            }
        }

        const results = [...this.#nodes.entries()]
            .filter(([stringId]) => !visited.has(stringId))
            .map(([, node]) => this.#removeNode(node.nodeId));
        return StateUpdate.merge(...results);
    }

    removeLink(sourceId: NodeId, targetId: NodeId): StateUpdate {
        return StateUpdate.merge(this.#removeLink(sourceId, targetId), this.#removeIsolatedNodes());
    }

    removeNode(nodeId: NodeId): StateUpdate {
        return StateUpdate.merge(this.#removeNode(nodeId), this.#removeIsolatedNodes());
    }

    getLinksNotYetFetchedNodes(): NodeId[] {
        return [...this.#nodes.values()].filter((node) => node.getLinks() === undefined).map((node) => node.nodeId);
    }

    getNodesNotYetFetchedCosts(): NodeId[] {
        return [...this.#nodes.values()].filter((node) => node.getCost() === undefined).map((node) => node.nodeId);
    }

    syncState(): StateUpdate {
        return new StateUpdate({
            nodeAdded: [...this.#nodes.values()].map((node) => ({ nodeId: node.nodeId, cost: node.getCost() })),
            linkAdded: [...this.#nodes.values()].flatMap((node) => {
                return (node.getStrongLinks() ?? []).map(([id, cost]) => ({ nodeId1: node.nodeId, nodeId2: id, cost }));
            }),
        });
    }
}
