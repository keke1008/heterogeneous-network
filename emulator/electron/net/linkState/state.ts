import { NodeId } from "@core/net";

type Id = string;

export type Link = { source: Id; target: Id };

export const toId = (id: NodeId): Id => id.toString();

export class ModifyResult {
    addedNodes: Id[];
    removedNodes: Id[];
    addedLinks: Link[];
    removedLinks: Link[];

    constructor(args?: { addedNodes?: Id[]; removedNodes?: Id[]; addedLinks?: Link[]; removedLinks?: Link[] }) {
        this.addedNodes = args?.addedNodes ?? [];
        this.removedNodes = args?.removedNodes ?? [];
        this.addedLinks = args?.addedLinks ?? [];
        this.removedLinks = args?.removedLinks ?? [];
    }

    static merge(...result: ModifyResult[]) {
        return new ModifyResult({
            addedNodes: result.flatMap((r) => r.addedNodes),
            removedNodes: result.flatMap((r) => r.removedNodes),
            addedLinks: result.flatMap((r) => r.addedLinks),
            removedLinks: result.flatMap((r) => r.removedLinks),
        });
    }
}

class NodeLinks {
    #strong: Set<Id> = new Set();
    #weak: Set<Id> = new Set();

    hasLink(nodeId: Id): boolean {
        return this.#strong.has(nodeId) || this.#weak.has(nodeId);
    }

    #linkAddable(nodeId: Id): boolean {
        return !this.hasLink(nodeId);
    }

    addStrongLink(nodeId: Id): "added" | "alreadyExists" {
        if (this.#linkAddable(nodeId)) {
            this.#strong.add(nodeId);
            return "added";
        } else {
            return "alreadyExists";
        }
    }

    addWeakLink(nodeId: Id): void {
        if (this.#linkAddable(nodeId)) {
            this.#weak.add(nodeId);
        }
    }

    removeLink(nodeId: Id): "removed" | "notExists" {
        if (this.#strong.delete(nodeId)) {
            return "removed";
        } else {
            this.#weak.delete(nodeId);
            return "notExists";
        }
    }

    getLinks(): Id[] {
        return [...this.#strong, ...this.#weak];
    }
}

class NetworkNode {
    readonly nodeId: NodeId;
    readonly id: Id;
    #links: NodeLinks | undefined;

    constructor(nodeId: NodeId) {
        this.nodeId = nodeId;
        this.id = toId(nodeId);
    }

    addStrongLink(nodeId: Id): ModifyResult {
        this.#links ??= new NodeLinks();
        const result = this.#links.addStrongLink(nodeId);
        if (result === "added") {
            return new ModifyResult({
                addedLinks: [{ source: this.id, target: nodeId }],
            });
        } else {
            return new ModifyResult();
        }
    }

    addWeakLink(nodeId: Id): void {
        this.#links ??= new NodeLinks();
        this.#links.addWeakLink(nodeId);
    }

    removeLink(nodeId: Id): ModifyResult {
        const result = this.#links?.removeLink(nodeId);
        if (result === "removed") {
            return new ModifyResult({
                removedLinks: [{ source: this.id, target: nodeId }],
            });
        } else {
            return new ModifyResult();
        }
    }

    getLinks(): Id[] | undefined {
        return this.#links?.getLinks();
    }
}

export class LinkState {
    #nodes: Map<Id, NetworkNode> = new Map();
    #selfId: NodeId;

    private constructor(selfId: NodeId) {
        this.#selfId = selfId;
    }

    static create(selfId: NodeId): [LinkState, ModifyResult] {
        const state = new LinkState(selfId);
        const result = state.createNode(selfId);
        return [state, result];
    }

    #getOrCreateNode(nodeId: NodeId): [NetworkNode, ModifyResult] {
        const node = this.#nodes.get(toId(nodeId));
        if (node === undefined) {
            const node = new NetworkNode(nodeId);
            this.#nodes.set(node.id, node);
            return [node, new ModifyResult({ addedNodes: [node.id] })];
        } else {
            return [node, new ModifyResult()];
        }
    }

    createNode(nodeId: NodeId): ModifyResult {
        return this.#getOrCreateNode(nodeId)[1];
    }

    createLink(sourceId: NodeId, targetId: NodeId): ModifyResult {
        const [source, result1] = this.#getOrCreateNode(sourceId);
        const [target, result2] = this.#getOrCreateNode(targetId);
        const result3 = source.addStrongLink(target.id);
        target.addWeakLink(source.id);
        return ModifyResult.merge(result1, result2, result3);
    }

    #removeLink(sourceId: Id, targetId: Id): ModifyResult {
        const source = this.#nodes.get(sourceId);
        const target = this.#nodes.get(targetId);
        if (source === undefined || target === undefined) {
            return new ModifyResult();
        } else {
            return ModifyResult.merge(source.removeLink(targetId), target.removeLink(sourceId));
        }
    }

    #removeNode(nodeId: Id): ModifyResult {
        const node = this.#nodes.get(nodeId);
        if (node === undefined) {
            return new ModifyResult();
        } else {
            const results = (node.getLinks() ?? [])
                .flatMap((id) => this.#nodes.get(id) ?? [])
                .map((node) => this.#removeLink(nodeId, node.id));

            this.#nodes.delete(nodeId);
            const result = new ModifyResult({ removedNodes: [nodeId] });
            return ModifyResult.merge(...results, result);
        }
    }

    #removeIsolatedNodes(): ModifyResult {
        const visited = new Set<Id>();
        const queue: Id[] = [toId(this.#selfId)];

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
            .map(([, node]) => this.#removeNode(node.id));
        return ModifyResult.merge(...results);
    }

    removeLink(sourceId: NodeId, targetId: NodeId): ModifyResult {
        return ModifyResult.merge(this.#removeLink(toId(sourceId), toId(targetId)), this.#removeIsolatedNodes());
    }

    removeNode(nodeId: NodeId): ModifyResult {
        return ModifyResult.merge(this.#removeNode(toId(nodeId)), this.#removeIsolatedNodes());
    }

    getLinksNotYetFetchedNodes(): NodeId[] {
        return [...this.#nodes.values()].filter((node) => node.getLinks() === undefined).map((node) => node.nodeId);
    }
}
