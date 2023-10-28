import { NodeId } from "@core/net";

export interface Graph {
    nodes: { id: string }[];
    links: { source: string; target: string }[];
}

type NodeStringId = string;

class NetworkNode {
    readonly nodeId: NodeId;
    readonly nodeStringId: NodeStringId;
    #links: Set<NodeStringId> | undefined;

    constructor(nodeId: NodeId) {
        this.nodeId = nodeId;
        this.nodeStringId = nodeId.toString();
    }

    stringId(): NodeStringId {
        return this.nodeStringId;
    }

    addLinkIfNotExists(nodeId: NodeId) {
        this.#links ??= new Set();
        if (!this.#links.has(nodeId.toString())) {
            this.#links.add(nodeId.toString());
        }
    }

    removeLink(...nodeIds: NodeId[]) {
        for (const id of nodeIds) {
            this.#links?.delete(id.toString());
        }
    }

    hasLink(nodeId: NodeId): boolean {
        return this.#links?.has(nodeId.toString()) ?? false;
    }

    getLinks(): Set<NodeStringId> | undefined {
        return this.#links;
    }
}

export class LinkState {
    #nodes: Map<NodeStringId, NetworkNode> = new Map();
    #selfId: NodeId;

    constructor(selfId: NodeId) {
        this.#selfId = selfId;
        this.#nodes.set(selfId.toString(), new NetworkNode(selfId));
    }

    #removeIsolatedNodes() {
        const visited = new Set<NodeStringId>();
        const queue: NodeStringId[] = [this.#selfId.toString()];

        while (queue.length > 0) {
            const id = queue.shift()!;
            visited.add(id);

            const node = this.#nodes.get(id)!;
            for (const link of node.getLinks()?.values() ?? []) {
                if (!visited.has(link)) {
                    queue.push(link);
                }
            }
        }

        for (const id of this.#nodes.keys()) {
            if (!visited.has(id)) {
                this.#nodes.delete(id);
            }
        }
    }

    getNode(nodeId: NodeId): NetworkNode | undefined {
        return this.#nodes.get(nodeId.toString());
    }

    hasNode(nodeId: NodeId): boolean {
        return this.getNode(nodeId) !== undefined;
    }

    getOrCreateNode(nodeId: NodeId): NetworkNode {
        const node = this.getNode(nodeId);
        if (node !== undefined) {
            return node;
        } else {
            const newNode = new NetworkNode(nodeId);
            this.#nodes.set(nodeId.toString(), newNode);
            return newNode;
        }
    }

    getLinksNotYetFetchedNodes(): NodeId[] {
        return [...this.#nodes.values()].filter((node) => node.getLinks() === undefined).map((node) => node.nodeId);
    }

    addNodeIfNotExists(nodeId: NodeId): void {
        if (this.hasNode(nodeId)) {
            this.#nodes.set(nodeId.toString(), new NetworkNode(nodeId));
        }
    }

    addLinkIfNotExists(nodeId: NodeId, linkId: NodeId) {
        const node = this.getOrCreateNode(nodeId);
        const link = this.getOrCreateNode(linkId);
        node.addLinkIfNotExists(linkId);
        link.addLinkIfNotExists(nodeId);
    }

    removeNode(nodeId: NodeId): void {
        const node = this.#nodes.get(nodeId.toString());
        if (node === undefined) {
            return;
        }

        node.getLinks()?.forEach((id) => {
            this.#nodes.get(id)?.removeLink(nodeId);
        });
        this.#nodes.delete(nodeId.toString());
        this.#removeIsolatedNodes();
    }

    removeLink(sourceId: NodeId, targetId: NodeId): void {
        const source = this.#nodes.get(sourceId.toString());
        const target = this.#nodes.get(targetId.toString());
        if (source === undefined || target === undefined) {
            return;
        }

        source.removeLink(targetId);
        target.removeLink(sourceId);
        this.#removeIsolatedNodes();
    }

    export(): Graph {
        const appended = new Set<NodeStringId>();
        const links: { source: string; target: string }[] = [];
        for (const node of this.#nodes.values()) {
            for (const link of node.getLinks() ?? []) {
                if (appended.has(link)) {
                    continue;
                }
                links.push({ source: node.stringId(), target: link });
            }
            appended.add(node.stringId());
        }

        return {
            nodes: [...this.#nodes.values()].map((node) => ({ id: node.stringId() })),
            links,
        };
    }
}
