import * as d3 from "d3";
import { Cost, NodeId, Source } from "@core/net";
import { NetworkUpdate } from "@core/net/observer/frame";
import { match } from "ts-pattern";
import { sleep } from "@core/async";
import { RECEIVED_HIGHLIGHT_COLOR, RECEIVED_HIGHLIGHT_TIMEOUT } from "./constants";
import { useMemo, useEffect } from "react";

export interface Node extends d3.SimulationNodeDatum {
    id: string;
    x: number;
    y: number;
    node: Source;
    cost?: Cost;
}

export interface Link extends d3.SimulationLinkDatum<Node> {
    source: string;
    target: string;
    x?: number;
    y?: number;
    angle?: number;
    cost: Cost;
    sourceNodeId: NodeId;
    targetNodeId: NodeId;
}

export interface Graph {
    render(nodes: Node[], links: Link[]): void;
    highlightNode(node: NodeId, color: string): void;
    clearHighlightNode(node: NodeId): void;
}

class NodeStore {
    #nodes: Map<string, Node> = new Map();

    static toId(nodeId: NodeId): string {
        return nodeId.toString();
    }

    get(nodeId: NodeId): Node | undefined {
        return this.#nodes.get(nodeId.toString());
    }

    nodes(): Node[] {
        return [...this.#nodes.values()];
    }

    update(node: Pick<Node, "node" | "cost">) {
        const id = NodeStore.toId(node.node.nodeId);
        const existing = this.#nodes.get(id);
        if (existing === undefined) {
            this.#nodes.set(id, { id, ...node, x: 0, y: 0 });
        } else {
            existing.cost = node.cost;
        }
    }

    remove(nodeId: NodeId) {
        this.#nodes.delete(nodeId.toString());
    }
}

class LinkStore {
    #links: Map<string, Link> = new Map();

    static #toId(source: NodeId, target: NodeId): string {
        return `${NodeStore.toId(source)}-${NodeStore.toId(target)}`;
    }

    get(source: NodeId, target: NodeId): Link | undefined {
        return this.#links.get(LinkStore.#toId(source, target));
    }

    links(): Link[] {
        return [...this.#links.values()];
    }

    createIfNotExists(link: Omit<Link, "source" | "target">) {
        const id = LinkStore.#toId(link.sourceNodeId, link.targetNodeId);
        if (!this.#links.has(id)) {
            this.#links.set(id, {
                ...link,
                source: NodeStore.toId(link.sourceNodeId),
                target: NodeStore.toId(link.targetNodeId),
            });
        }
    }

    remove(source: NodeId, target: NodeId) {
        this.#links.delete(LinkStore.#toId(source, target));
    }
}

export class GraphControl {
    #nodes = new NodeStore();
    #links = new LinkStore();
    #graph: Graph;

    constructor(graph: Graph) {
        this.#graph = graph;
    }

    render() {
        this.#graph.render(this.#nodes.nodes(), this.#links.links());
    }

    applyNetworkUpdates(updates: NetworkUpdate[]) {
        for (const update of updates) {
            match(update)
                .with({ type: "NodeUpdated" }, ({ node, cost }) => {
                    const exists = this.#nodes.get(node.nodeId);
                    if (exists === undefined) {
                        this.#nodes.update({ node, cost });
                    } else {
                        exists.cost = cost;
                    }
                })
                .with({ type: "NodeRemoved" }, ({ id }) => {
                    this.#nodes.remove(id);
                })
                .with({ type: "LinkUpdated" }, ({ source, destination, sourceCost, destinationCost, linkCost }) => {
                    this.#nodes.update({ node: source, cost: sourceCost });
                    this.#nodes.update({ node: destination, cost: destinationCost });
                    this.#links.createIfNotExists({
                        cost: linkCost,
                        sourceNodeId: source.nodeId,
                        targetNodeId: destination.nodeId,
                    });
                })
                .with({ type: "LinkRemoved" }, ({ sourceId, destinationId }) => {
                    this.#links.remove(sourceId, destinationId);
                })
                .with({ type: "FrameReceived" }, async ({ receivedNodeId }) => {
                    this.#graph.highlightNode(receivedNodeId, RECEIVED_HIGHLIGHT_COLOR);
                    await sleep(RECEIVED_HIGHLIGHT_TIMEOUT);
                    this.#graph.clearHighlightNode(receivedNodeId);
                })
                .exhaustive();
        }

        this.render();
    }
}

export const useGraphControl = (graph: Graph) => {
    const graphControl = useMemo(() => {
        return new GraphControl(graph);
    }, [graph]);

    useEffect(() => {
        graphControl.render();
    }, [graphControl]);

    return {
        applyNetworkUpdates(updates: NetworkUpdate[]) {
            graphControl.applyNetworkUpdates(updates);
        },
    };
};
