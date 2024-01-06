import * as d3 from "d3";
import { Cost, NodeId, Source } from "@core/net";
import { NetworkUpdate } from "@core/net/observer/frame";
import { match } from "ts-pattern";
import { COLOR, RECEIVED_HIGHLIGHT_TIMEOUT } from "./constants";
import { useEffect, MutableRefObject, useRef } from "react";
import { ObjectMap, ObjectSet } from "@core/object";
import { Duration } from "@core/time";

export class Node implements d3.SimulationNodeDatum {
    id: string;
    x: number = 0;
    y: number = 0;
    fx?: number;
    fy?: number;
    source: Source;
    cost: Cost;

    constructor(args: { source: Source; cost: Cost }) {
        this.id = args.source.nodeId.uniqueKey();
        this.source = args.source;
        this.cost = args.cost;
    }

    uniqueKey(): string {
        return this.id;
    }
}

export class Link implements d3.SimulationLinkDatum<Node> {
    source: string;
    target: string;
    x?: number;
    y?: number;
    angle?: number;
    cost: Cost;
    sourceNodeId: NodeId;
    targetNodeId: NodeId;

    static uniqueKey(source: NodeId, target: NodeId): string {
        return `${source.uniqueKey()}-${target.uniqueKey()}`;
    }

    constructor(args: { sourceNodeId: NodeId; targetNodeId: NodeId; cost: Cost }) {
        this.source = args.sourceNodeId.uniqueKey();
        this.target = args.targetNodeId.uniqueKey();
        this.cost = args.cost;
        this.sourceNodeId = args.sourceNodeId;
        this.targetNodeId = args.targetNodeId;
    }

    uniqueKey(): string {
        return Link.uniqueKey(this.sourceNodeId, this.targetNodeId);
    }

    calculateParameters(source: Node, target: Node) {
        const { x: x1, y: y1 } = source;
        const { x: x2, y: y2 } = target;
        this.x = (x1 + x2) / 2;
        this.y = (y1 + y2) / 2;

        this.angle = Math.atan2(y2 - y1, x2 - x1);
        if (Math.abs(this.angle) > Math.PI / 2) {
            this.angle += Math.PI;
        }
    }

    transform(): string {
        return `rotate(${(this.angle! * 180) / Math.PI}, ${this.x}, ${this.y})`;
    }
}

export interface Graph {
    render(nodes: Node[], links: Link[]): void;
    setNodeColor(node: NodeId, color: string): void;
    onClickNode(callback: (node: Source) => void): void;
    onClickOutsideNode(callback: () => void): void;
}

class NodeStore {
    #nodes = new ObjectSet<Node>();

    static toId(nodeId: NodeId): string {
        return nodeId.toString();
    }

    get(nodeId: NodeId): Node | undefined {
        return this.#nodes.getByKey(nodeId.uniqueKey());
    }

    nodes(): Node[] {
        return [...this.#nodes.values()];
    }

    update(node: Pick<Node, "source" | "cost">) {
        const existing = this.#nodes.getByKey(node.source.nodeId.uniqueKey());
        if (existing === undefined) {
            this.#nodes.add(new Node(node));
        } else {
            existing.cost = node.cost;
        }
    }

    remove(nodeId: NodeId) {
        this.#nodes.deleteByKey(nodeId.uniqueKey());
    }
}

class LinkStore {
    #links = new ObjectSet<Link>();

    get(source: NodeId, target: NodeId): Link | undefined {
        return this.#links.getByKey(Link.uniqueKey(source, target));
    }

    links(): Link[] {
        return [...this.#links.values()];
    }

    createIfNotExists(link: { sourceNodeId: NodeId; targetNodeId: NodeId; cost: Cost }) {
        if (!this.#links.hasByKey(Link.uniqueKey(link.sourceNodeId, link.targetNodeId))) {
            this.#links.add(
                new Link({
                    sourceNodeId: link.sourceNodeId,
                    targetNodeId: link.targetNodeId,
                    cost: link.cost,
                }),
            );
        }
    }

    remove(source: NodeId, target: NodeId) {
        this.#links.deleteByKey(Link.uniqueKey(source, target));
    }
}

class HighlightManager {
    #graph: Graph;
    #instantHighlight = new ObjectMap<NodeId, { color: string; cancelReset: () => void }>();
    #permanentHighlight = new ObjectMap<NodeId, string>();

    constructor(graph: Graph) {
        this.#graph = graph;
    }

    highlight(nodeId: NodeId, color: string, duration: Duration) {
        this.#instantHighlight.get(nodeId)?.cancelReset();

        const id = setTimeout(() => {
            const resetColor = this.#permanentHighlight.get(nodeId) ?? COLOR.DEFAULT;
            this.#graph.setNodeColor(nodeId, resetColor);
            this.#instantHighlight.delete(nodeId);
        }, duration.millies);
        this.#instantHighlight.set(nodeId, { color, cancelReset: () => clearTimeout(id) });

        this.#graph.setNodeColor(nodeId, color);
    }

    permanentHighlight(nodeId: NodeId, color: string) {
        this.#permanentHighlight.set(nodeId, color);
        this.#graph.setNodeColor(nodeId, color);
    }

    clearPermanentHighlight(color: string) {
        for (const nodeId of this.#permanentHighlight.keys()) {
            if (this.#permanentHighlight.get(nodeId) !== color) {
                continue;
            }

            this.#permanentHighlight.delete(nodeId);
            const resetColor = this.#instantHighlight.get(nodeId)?.color ?? COLOR.DEFAULT;
            this.#graph.setNodeColor(nodeId, resetColor);
        }
        return;
    }
}

export class GraphControl {
    #nodes = new NodeStore();
    #links = new LinkStore();
    #highlight: HighlightManager;
    #graph: Graph;

    constructor(graph: Graph) {
        this.#highlight = new HighlightManager(graph);
        this.#graph = graph;
        this.#graph.onClickNode((node) => {
            this.#highlight.clearPermanentHighlight(COLOR.SELECTED);
            this.#highlight.permanentHighlight(node.nodeId, COLOR.SELECTED);
        });
        this.#graph.onClickOutsideNode(() => this.#highlight.clearPermanentHighlight(COLOR.SELECTED));
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
                        this.#nodes.update({ source: node, cost });
                    } else {
                        exists.cost = cost;
                    }
                })
                .with({ type: "NodeRemoved" }, ({ id }) => {
                    this.#nodes.remove(id);
                })
                .with({ type: "LinkUpdated" }, ({ source, destination, sourceCost, destinationCost, linkCost }) => {
                    this.#nodes.update({ source: source, cost: sourceCost });
                    this.#nodes.update({ source: destination, cost: destinationCost });
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
                    this.#highlight.highlight(receivedNodeId, COLOR.RECEIVED, RECEIVED_HIGHLIGHT_TIMEOUT);
                })
                .exhaustive();
        }

        this.render();
    }
}

export const useGraphControl = (graphRef: MutableRefObject<Graph | undefined>) => {
    const controlRef = useRef<GraphControl>();

    useEffect(() => {
        if (graphRef.current === undefined) {
            throw new Error("Graph is not initialized");
        }

        controlRef.current = new GraphControl(graphRef.current!);
        controlRef.current.render();
    }, [graphRef]);

    return {
        applyNetworkUpdates(updates: NetworkUpdate[]) {
            controlRef.current!.applyNetworkUpdates(updates);
        },
    };
};
