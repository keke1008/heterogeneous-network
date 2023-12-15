import { Cost, NetworkUpdate, NodeId } from "@core/net";
import * as d3 from "d3";
import { useEffect, useRef } from "react";
import { match } from "ts-pattern";

interface Node extends d3.SimulationNodeDatum {
    id: string;
    x: number;
    y: number;
    nodeId: NodeId;
    cost?: Cost;
}

interface Link extends d3.SimulationLinkDatum<Node> {
    source: string;
    target: string;
    x?: number;
    y?: number;
    angle?: number;
    cost: Cost;
    sourceNodeId: NodeId;
    targetNodeId: NodeId;
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

    update(node: Pick<Node, "nodeId" | "cost">) {
        const id = NodeStore.toId(node.nodeId);
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

class GraphState {
    #nodes = new NodeStore();
    #links = new LinkStore();

    nodes(): Node[] {
        return this.#nodes.nodes();
    }

    links(): Link[] {
        return this.#links.links();
    }

    applyNetworkUpdates(updates: NetworkUpdate[]) {
        for (const update of updates) {
            match(update)
                .with({ type: "NodeUpdated" }, ({ id, cost }) => {
                    const node = this.#nodes.get(id);
                    if (node === undefined) {
                        this.#nodes.update({ nodeId: id, cost });
                    } else {
                        node.cost = cost;
                    }
                })
                .with({ type: "NodeRemoved" }, ({ id }) => {
                    this.#nodes.remove(id);
                })
                .with({ type: "LinkUpdated" }, ({ sourceId, destinationId, sourceCost, destinationCost, linkCost }) => {
                    this.#nodes.update({ nodeId: sourceId, cost: sourceCost });
                    this.#nodes.update({ nodeId: destinationId, cost: destinationCost });
                    this.#links.createIfNotExists({
                        cost: linkCost,
                        sourceNodeId: sourceId,
                        targetNodeId: destinationId,
                    });
                })
                .with({ type: "LinkRemoved" }, ({ sourceId, destinationId }) => {
                    this.#links.remove(sourceId, destinationId);
                })
                .exhaustive();
        }
    }
}

interface EventHandler {
    onClickNode?: (props: { element: SVGGElement; data: Node }) => void;
    onClickOutsideNode?: () => void;
}

class Renderer {
    // CSSで100%にしているので、ここで指定した値は画面上の大きさには影響しない
    #width: number = 500;
    #height: number = 500;
    #nodeRadius: number;
    #eventHandler: EventHandler;

    #root: d3.Selection<SVGSVGElement, unknown, null, undefined>;
    #linkRoot: d3.Selection<SVGGElement, unknown, null, undefined>;
    #nodeRoot: d3.Selection<SVGGElement, unknown, null, undefined>;
    #simulation: d3.Simulation<Node, Link>;

    constructor(args: {
        parent: HTMLElement;
        nodeRadius: number;
        nodesData: Node[];
        linksData: Link[];
        eventHandler: EventHandler;
    }) {
        this.#nodeRadius = args.nodeRadius;
        this.#eventHandler = args.eventHandler;

        this.#root = d3
            .select(args.parent)
            .append("svg")
            .attr("width", this.#width)
            .attr("height", this.#height)
            .attr("viewBox", `0 0 ${this.#width} ${this.#height}`)
            .style("width", "100%")
            .style("height", "100%");
        this.#linkRoot = this.#root.append("g").classed("links", true);
        this.#nodeRoot = this.#root.append("g").classed("nodes", true);
        this.#root.on("click", function () {
            args.eventHandler.onClickOutsideNode?.();
        });

        const clampX = (x: number) => Math.max(this.#nodeRadius, Math.min(this.#width - this.#nodeRadius, x));
        const clampY = (y: number) => Math.max(this.#nodeRadius, Math.min(this.#height - this.#nodeRadius, y));

        const centerX = this.#width / 2;
        const centerY = this.#height / 2;

        this.#simulation = d3
            .forceSimulation<Node, Link>(args.nodesData)
            .force("center", d3.forceCenter(centerX, centerY))
            .force("collision", d3.forceCollide(args.nodeRadius))
            .on("tick", () => {
                const nodeMap: Map<string, Node> = new Map();
                const node = (id: string) => nodeMap.get(id)!;

                this.#nodeRoot
                    .selectAll<SVGGElement, Node>("g")
                    .each((d) => {
                        d.x = clampX(d.x);
                        d.y = clampY(d.y);
                        nodeMap.set(d.id, d);
                    })
                    .attr("transform", (d) => `translate(${d.x}, ${d.y})`);

                const linksGroup = this.#linkRoot.selectAll<SVGGElement, Link>("g");
                linksGroup
                    .select<SVGLineElement>("line")
                    .attr("x1", (d) => node(d.source).x!)
                    .attr("y1", (d) => node(d.source).y!)
                    .attr("x2", (d) => node(d.target).x!)
                    .attr("y2", (d) => node(d.target).y!);
                linksGroup
                    .select<SVGTextElement>("text")
                    .each((d) => {
                        const x1 = node(d.source).x!;
                        const y1 = node(d.source).y!;
                        const x2 = node(d.target).x!;
                        const y2 = node(d.target).y!;
                        d.x = (x1 + x2) / 2;
                        d.y = (y1 + y2) / 2;
                        d.angle = Math.atan2(y2 - y1, x2 - x1);
                        if (Math.abs(d.angle) > Math.PI / 2) {
                            d.angle += Math.PI;
                        }
                    })
                    .attr("x", (d) => d.x!)
                    .attr("y", (d) => d.y!)
                    .attr("transform", (d) => {
                        return `rotate(${(d.angle! * 180) / Math.PI}, ${d.x!}, ${d.y!})`;
                    });
            });
    }

    render(nodesData: Node[], linksData: Link[]) {
        const eventHandler = this.#eventHandler;

        this.#simulation.nodes(nodesData);

        const links = this.#linkRoot.selectAll("g").data(linksData);
        const linksGroup = links.enter().append("g");
        linksGroup.append("line").style("stroke", "black").style("stroke-width", 1);
        linksGroup
            .append("text")
            .attr("text-anchor", "middle")
            .text((link) => link.cost.get());
        links.exit().remove();

        const nodes = this.#nodeRoot.selectAll<SVGGElement, Node>("g").data(nodesData, (d: Node) => d.id);
        const nodesGroup = nodes.enter().append("g");
        nodesGroup.on("click", function (event, data) {
            (event as Event).stopPropagation();
            eventHandler.onClickNode?.({ element: this, data });
        });
        nodesGroup.append("circle").attr("r", this.#nodeRadius).style("fill", "lime");
        nodesGroup
            .append("text")
            .attr("text-anchor", "middle")
            .attr("dominant-baseline", "middle")
            .text((node) => node.cost?.get() ?? "?");
        nodesGroup.call(
            d3
                .drag<SVGGElement, Node>()
                .on("start", (event, d) => {
                    if (!event.active) {
                        this.#simulation.alphaTarget(0.3).restart();
                        d.fx = d.x;
                        d.fy = d.y;
                    }
                })
                .on("drag", (event: d3.D3DragEvent<SVGGElement, Node, unknown>, d) => {
                    d.fx = event.x;
                    d.fy = event.y;
                })
                .on("end", (event, d) => {
                    if (!event.active) {
                        this.#simulation.alphaTarget(0);
                    }
                    d.fx = null;
                    d.fy = null;
                }),
        );
        nodes.exit().remove();

        this.#simulation.alphaTarget(0.3).restart();
    }

    onDispose() {
        this.#simulation.stop();
        this.#root.remove();
    }
}

export interface NodeClickEvent {
    nodeId: NodeId;
}

export interface Props {
    rootRef: React.RefObject<HTMLElement>;
    onClickNode?: (event: NodeClickEvent) => void;
    onClickOutsideNode?: () => void;
}

export const useGraphRenderer = (props: Props) => {
    const { rootRef, onClickNode, onClickOutsideNode } = props;

    const graphStateRef = useRef<GraphState>();
    const rendererRef = useRef<Renderer>();

    const render = () => {
        rendererRef.current?.render(graphStateRef.current?.nodes() ?? [], graphStateRef.current?.links() ?? []);
    };

    useEffect(() => {
        if (rootRef.current === null) {
            throw new Error("rootRef.current is null");
        }

        const nodeRadius = 10;

        const eventHandler: EventHandler = {
            onClickNode: ({ data }) => onClickNode?.({ nodeId: data.nodeId }),
            onClickOutsideNode,
        };

        graphStateRef.current = new GraphState();
        rendererRef.current = new Renderer({
            linksData: graphStateRef.current.links(),
            nodesData: graphStateRef.current.nodes(),
            parent: rootRef.current,
            nodeRadius,
            eventHandler,
        });
        render();

        return () => {
            rendererRef.current?.onDispose();
            rendererRef.current = undefined;
            graphStateRef.current = undefined;
        };
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [rootRef]);

    const applyUpdates = (updates: NetworkUpdate[]) => {
        graphStateRef.current?.applyNetworkUpdates(updates);
        render();
    };

    return { applyUpdates };
};
