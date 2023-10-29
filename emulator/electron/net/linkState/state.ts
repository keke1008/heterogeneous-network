import { NodeId } from "@core/net";

type NodeStringId = string;

export type Link = { source: NodeId; target: NodeId };

export class ModifyResult {
    addedNodes: NodeId[];
    removedNodes: NodeId[];
    addedLinks: Link[];
    removedLinks: Link[];

    constructor(args?: { addedNodes?: NodeId[]; removedNodes?: NodeId[]; addedLinks?: Link[]; removedLinks?: Link[] }) {
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
    #strong: Set<NodeStringId> = new Set();
    #weak: Set<NodeStringId> = new Set();

    hasLink(nodeId: NodeId): boolean {
        return this.#strong.has(nodeId.toString()) || this.#weak.has(nodeId.toString());
    }

    #linkAddable(nodeId: NodeId): boolean {
        return !this.hasLink(nodeId);
    }

    addStrongLink(nodeId: NodeId): "added" | "alreadyExists" {
        if (this.#linkAddable(nodeId)) {
            this.#strong.add(nodeId.toString());
            return "added";
        } else {
            return "alreadyExists";
        }
    }

    addWeakLink(nodeId: NodeId): void {
        if (this.#linkAddable(nodeId)) {
            this.#weak.add(nodeId.toString());
        }
    }

    removeLink(nodeId: NodeId): "removed" | "notExists" {
        if (this.#strong.delete(nodeId.toString())) {
            return "removed";
        } else {
            this.#weak.delete(nodeId.toString());
            return "notExists";
        }
    }

    getLinks(): NodeStringId[] {
        return [...this.#strong, ...this.#weak];
    }
}

class NetworkNode {
    readonly nodeId: NodeId;
    readonly nodeStringId: NodeStringId;
    #links: NodeLinks | undefined;

    constructor(nodeId: NodeId) {
        this.nodeId = nodeId;
        this.nodeStringId = nodeId.toString();
    }

    stringId(): NodeStringId {
        return this.nodeStringId;
    }

    addStrongLink(nodeId: NodeId): ModifyResult {
        const result = this.#links?.addStrongLink(nodeId);
        if (result === "added") {
            return new ModifyResult({ addedLinks: [{ source: this.nodeId, target: nodeId }] });
        } else {
            return new ModifyResult();
        }
    }

    addWeakLink(nodeId: NodeId): void {
        this.#links?.addWeakLink(nodeId);
    }

    removeLink(nodeId: NodeId): ModifyResult {
        const result = this.#links?.removeLink(nodeId);
        if (result === "removed") {
            return new ModifyResult({ removedLinks: [{ source: this.nodeId, target: nodeId }] });
        } else {
            return new ModifyResult();
        }
    }

    getLinks(): NodeStringId[] | undefined {
        return this.#links?.getLinks();
    }
}

export class LinkState {
    #nodes: Map<NodeStringId, NetworkNode> = new Map();
    #selfId: NodeId;

    constructor(selfId: NodeId) {
        this.#selfId = selfId;
    }

    #getOrCreateNode(nodeId: NodeId): [NetworkNode, ModifyResult] {
        const node = this.#nodes.get(nodeId.toString());
        if (node === undefined) {
            const node = new NetworkNode(nodeId);
            this.#nodes.set(nodeId.toString(), node);
            return [node, new ModifyResult({ addedNodes: [nodeId] })];
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
        const result3 = source.addStrongLink(targetId);
        target.addWeakLink(sourceId);
        return ModifyResult.merge(result1, result2, result3);
    }

    #removeLink(sourceId: NodeId, targetId: NodeId): ModifyResult {
        const source = this.#nodes.get(sourceId.toString());
        const target = this.#nodes.get(targetId.toString());
        if (source === undefined || target === undefined) {
            return new ModifyResult();
        } else {
            return ModifyResult.merge(source.removeLink(targetId), target.removeLink(sourceId));
        }
    }

    #removeNode(nodeId: NodeId): ModifyResult {
        const node = this.#nodes.get(nodeId.toString());
        if (node === undefined) {
            return new ModifyResult();
        } else {
            const results = (node.getLinks() ?? [])
                .flatMap((id) => this.#nodes.get(id) ?? [])
                .map((node) => this.#removeLink(nodeId, node.nodeId));

            this.#nodes.delete(nodeId.toString());
            const result = new ModifyResult({ removedNodes: [nodeId] });
            return ModifyResult.merge(...results, result);
        }
    }

    #removeIsolatedNodes(): ModifyResult {
        const visited = new Set<NodeStringId>();
        const queue: NodeStringId[] = [this.#selfId.toString()];

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
        return ModifyResult.merge(...results);
    }

    removeLink(sourceId: NodeId, targetId: NodeId): ModifyResult {
        return ModifyResult.merge(this.#removeLink(sourceId, targetId), this.#removeIsolatedNodes());
    }

    removeNode(nodeId: NodeId): ModifyResult {
        return ModifyResult.merge(this.#removeNode(nodeId), this.#removeIsolatedNodes());
    }

    getLinksNotYetFetchedNodes(): NodeId[] {
        return [...this.#nodes.values()].filter((node) => node.getLinks() === undefined).map((node) => node.nodeId);
    }
}
