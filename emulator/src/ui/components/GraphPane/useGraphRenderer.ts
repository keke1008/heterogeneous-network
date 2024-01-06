import { NodeId, Source } from "@core/net";
import * as d3 from "d3";
import { useEffect, useRef } from "react";
import { Node, Link, Graph } from "./useGraphControl";

interface EventHandler {
    onClickNode?: (props: { element: SVGGElement; data: Node }) => void;
    onClickOutsideNode?: () => void;
}

class Renderer implements Graph {
    // CSSで100%にしているので、ここで指定した値は画面上の大きさには影響しない
    #width: number = 500;
    #height: number = 500;
    #nodeRadius: number;
    #eventHandler: EventHandler;

    #root: d3.Selection<SVGSVGElement, unknown, null, undefined>;
    #linkRoot: d3.Selection<SVGGElement, unknown, null, undefined>;
    #nodeRoot: d3.Selection<SVGGElement, unknown, null, undefined>;
    #simulation: d3.Simulation<Node, Link>;

    constructor(args: { parent: HTMLElement; nodeRadius: number; eventHandler: EventHandler }) {
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
            .forceSimulation<Node, Link>()
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
                    .each((d) => d.calculateParameters(node(d.source), node(d.target)))
                    .attr("x", (d) => d.x!)
                    .attr("y", (d) => d.y!)
                    .attr("transform", (d) => d.transform());
            });
    }

    render(nodesData: Node[], linksData: Link[]) {
        const eventHandler = this.#eventHandler;

        this.#simulation.nodes(nodesData);

        const links = this.#linkRoot.selectAll("g").data(linksData);
        const linksGroup = links.enter().append("g");
        linksGroup.append("line").style("stroke", "white").style("stroke-width", 1);
        linksGroup
            .append("text")
            .attr("text-anchor", "middle")
            .style("fill", "white")
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
                    d.fx = undefined;
                    d.fy = undefined;
                }),
        );
        nodes.exit().remove();

        this.#simulation.alphaTarget(0.3).restart();
    }

    highlightNode(nodeId: NodeId, color: string) {
        this.#nodeRoot
            .selectAll<SVGGElement, Node>("g")
            .filter((d) => d.source.nodeId.equals(nodeId))
            .select("circle")
            .style("fill", color);
    }

    clearHighlightNode(nodeId: NodeId) {
        this.#nodeRoot
            .selectAll<SVGGElement, Node>("g")
            .filter((d) => d.source.nodeId.equals(nodeId))
            .select("circle")
            .style("fill", "lime");
    }

    onDispose() {
        this.#simulation.stop();
        this.#root.remove();
    }
}

export interface NodeClickEvent {
    node: Source;
}

export interface Props {
    rootRef: React.RefObject<HTMLElement>;
    onClickNode?: (event: NodeClickEvent) => void;
    onClickOutsideNode?: () => void;
}

export const useGraphRenderer = (props: Props) => {
    const { rootRef, onClickNode, onClickOutsideNode } = props;

    const rendererRef = useRef<Renderer>();
    useEffect(() => {
        rendererRef.current = new Renderer({
            parent: rootRef.current!,
            nodeRadius: 10,
            eventHandler: {
                onClickNode: ({ data }) => onClickNode?.({ node: data.source }),
                onClickOutsideNode,
            },
        });

        return () => {
            rendererRef.current?.onDispose();
        };
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [rootRef]);

    return { rendererRef };
};
