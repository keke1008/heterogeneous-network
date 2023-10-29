import { NodeId } from "@core/net";
import * as d3 from "d3";
import type { ModifyResult } from "emulator/electron/net";
import { useEffect, useRef } from "react";

const toId = (id: NodeId) => id.toString();

interface Node extends d3.SimulationNodeDatum {
    id: string;
    x: number;
    y: number;
}

interface Link extends d3.SimulationLinkDatum<Node> {
    source: string;
    target: string;
    x?: number;
    y?: number;
    angle?: number;
}

class GraphState {
    #nodes: Node[] = [];
    #links: Link[] = [];
    #centerX: number;
    #centerY: number;

    constructor({ width, height }: { width: number; height: number }) {
        this.#centerX = width;
        this.#centerY = height;
    }

    nodes(): Node[] {
        return this.#nodes;
    }

    links(): Link[] {
        return this.#links;
    }

    applyModification(modifyResult: ModifyResult) {
        modifyResult.addedNodes.forEach((node) =>
            this.#nodes.push({ id: toId(node), x: this.#centerX / 2, y: this.#centerY / 2 }),
        );
        modifyResult.addedLinks.forEach(({ source, target }) =>
            this.#links.push({ source: toId(source), target: toId(target) }),
        );
        modifyResult.removedNodes.forEach((node) => {
            const index = this.#nodes.findIndex((n) => n.id === toId(node));
            if (index >= 0) {
                this.#nodes.splice(index, 1);
            }
        });

        const removedNodes: Set<string> = new Set();
        modifyResult.removedNodes.forEach((node) => removedNodes.add(toId(node)));
        this.#nodes = this.#nodes.filter(({ id }) => !removedNodes.has(id));

        const removedLinks: Map<string, string> = new Map();
        modifyResult.removedLinks.forEach(({ source, target }) => removedLinks.set(toId(source), toId(target)));
        this.#links = this.#links.filter(({ source, target }) => {
            return removedLinks.get(source) !== target;
        });
    }
}

class Renderer {
    #width: number;
    #height: number;
    #nodeRadius: number;

    #root: d3.Selection<SVGSVGElement, unknown, null, undefined>;
    #linkRoot: d3.Selection<SVGGElement, unknown, null, undefined>;
    #nodeRoot: d3.Selection<SVGGElement, unknown, null, undefined>;
    #simulation: d3.Simulation<Node, Link>;

    constructor(args: {
        parent: HTMLElement;
        width: number;
        height: number;
        nodeRadius: number;
        nodesData: Node[];
        linksData: Link[];
    }) {
        this.#width = args.width;
        this.#height = args.height;
        this.#nodeRadius = args.nodeRadius;

        this.#root = d3
            .select(args.parent)
            .append("svg")
            .attr("width", args.width)
            .attr("height", args.height)
            .style("border", "1px solid black");
        this.#linkRoot = this.#root.append("g").classed("links", true);
        this.#nodeRoot = this.#root.append("g").classed("nodes", true);

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
                    .attr("text-anchor", "middle")
                    .attr("transform", (d) => {
                        return `rotate(${(d.angle! * 180) / Math.PI}, ${d.x!}, ${d.y!})`;
                    });
            });
    }

    render(nodesData: Node[], linksData: Link[]) {
        this.#simulation.nodes(nodesData);

        const links = this.#linkRoot.selectAll("g").data(linksData);
        const linksGroup = links.enter().append("g");
        linksGroup.append("line").style("stroke", "black").style("stroke-width", 1);
        linksGroup.append("text").text((_, i) => i);
        links.exit().remove();

        const nodes = this.#nodeRoot.selectAll<SVGGElement, Node>("g").data(nodesData, (d: Node) => d.id);
        const nodesGroup = nodes.enter().append("g");
        nodesGroup.append("circle").attr("r", this.#nodeRadius).style("fill", "lime");
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
    }

    onDispose() {
        this.#simulation.stop();
        this.#root.remove();
    }
}

export const Graph = () => {
    const rootRef = useRef<HTMLDivElement>(null);

    useEffect(() => {
        const width = 500;
        const height = 500;
        const nodeRadius = 10;

        const state = new GraphState({ width, height });
        const renderer = new Renderer({
            linksData: state.links(),
            nodesData: state.nodes(),
            parent: rootRef.current!,
            width,
            height,
            nodeRadius,
        });

        window.net.onGraphModified((_event, result) => {
            state.applyModification(result);
            renderer.render(state.nodes(), state.links());
        });

        return () => {
            renderer.onDispose();
        };
    }, []);

    return <div ref={rootRef}></div>;
};
