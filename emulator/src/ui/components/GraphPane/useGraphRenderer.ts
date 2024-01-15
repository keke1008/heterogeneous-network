import { NodeId, PartialNode } from "@core/net";
import * as d3 from "d3";
import { useEffect, useRef } from "react";
import { Node, Link, Graph } from "./useGraphControl";
import { COLOR } from "./constants";
import { CancelListening, EventBroker } from "@core/event";

class Renderer implements Graph {
    // CSSで100%にしているので、ここで指定した値は画面上の大きさには影響しない
    #size: number = 800;
    #nodeRadius: number;

    #root: d3.Selection<SVGSVGElement, unknown, null, undefined>;
    #linkRoot: d3.Selection<SVGGElement, unknown, null, undefined>;
    #nodeRoot: d3.Selection<SVGGElement, unknown, null, undefined>;
    #simulation: d3.Simulation<Node, Link>;

    #onClickNode = new EventBroker<PartialNode>();
    #onClickOutsideNode = new EventBroker<void>();

    constructor(args: { parent: HTMLElement; nodeRadius: number }) {
        this.#nodeRadius = args.nodeRadius;

        this.#root = d3
            .select(args.parent)
            .append("svg")
            .attr("width", this.#size)
            .attr("height", this.#size)
            .attr("viewBox", `${-this.#size / 2} ${-this.#size / 2} ${this.#size} ${this.#size}`)
            .style("width", "100%")
            .style("height", "100%");
        this.#linkRoot = this.#root.append("g").classed("links", true);
        this.#nodeRoot = this.#root.append("g").classed("nodes", true);
        this.#root.on("click", () => this.#onClickOutsideNode.emit());

        const clamp = (() => {
            const min = -this.#size / 2 + this.#nodeRadius;
            const max = -min;
            return (x: number) => Math.max(min, Math.min(max, x));
        })();

        this.#simulation = d3
            .forceSimulation<Node, Link>()
            .force("collision", d3.forceCollide(args.nodeRadius))
            .on("tick", () => {
                const nodeMap: Map<string, Node> = new Map();
                const node = (id: string) => nodeMap.get(id)!;

                this.#nodeRoot
                    .selectAll<SVGGElement, Node>("g")
                    .each((d) => {
                        d.x = clamp(d.x);
                        d.y = clamp(d.y);
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
        this.#simulation.nodes(nodesData);

        // リンクの描画
        const linkGroupUpdate = this.#linkRoot.selectAll<SVGGElement, Link>("g").data(linksData);
        linkGroupUpdate.exit().remove();
        const linkGroupEnter = linkGroupUpdate.enter().append("g");
        const linkGroup = linkGroupEnter.merge(linkGroupUpdate);

        // リンクの線の描画
        const linkLineUpdate = linkGroup.selectAll<SVGLineElement, Link>("line").data((link) => [link.cost]);
        const linkLineEnter = linkLineUpdate.enter().append("line").style("stroke", "white").style("stroke-width", 1);
        linkLineUpdate.merge(linkLineEnter).text((cost) => cost.get());

        // リンクのコストの描画
        const linkTextUpdate = linkGroup.selectAll<SVGTextElement, Link>("text").data((link) => [link.cost]);
        const linkTextEnter = linkTextUpdate
            .enter()
            .append("text")
            .attr("text-anchor", "middle")
            .style("fill", "white");
        linkTextUpdate.merge(linkTextEnter).text((cost) => cost.get());

        // ノードの描画
        const nodeGroupUpdate = this.#nodeRoot.selectAll<SVGGElement, Node>("g").data(nodesData, (d: Node) => d.id);
        nodeGroupUpdate.exit().remove();
        const nodeGroupEnter = nodeGroupUpdate.enter().append("g");
        nodeGroupEnter
            .on("click", (event, node) => {
                (event as Event).stopPropagation();
                this.#onClickNode.emit(node.node);
            })
            .append("circle")
            .attr("r", this.#nodeRadius)
            .style("fill", COLOR.DEFAULT);
        const nodeGroup = nodeGroupEnter.merge(nodeGroupUpdate);

        // ノードのコストの描画
        const nodeCostTextUpdate = nodeGroup
            .selectAll<SVGTextElement, Node>("text.node-cost")
            .data(({ node }) => [node.cost]);
        const nodeCostTextEnter = nodeCostTextUpdate
            .enter()
            .append("text")
            .classed("node-cost", true)
            .attr("text-anchor", "middle")
            .attr("dominant-baseline", "middle");
        nodeCostTextUpdate.merge(nodeCostTextEnter).text((cost) => cost?.get() ?? "?");

        // ノードの右下のテキストの描画
        const nodePropertyTextUpdate = nodeGroup
            .selectAll<SVGTextElement, Node>("text.property")
            .data(({ node }) => [
                [`ID: ${node.nodeId.toHumanReadableString()}`, `Cluster: ${node.clusterId?.display() ?? "?"}`],
            ]);
        const nodePropertyTextEnter = nodePropertyTextUpdate
            .enter()
            .append("text")
            .classed("property", true)
            .attr("fill", "white")
            .attr("x", this.#nodeRadius)
            .attr("y", this.#nodeRadius);
        const nodePropertyText = nodePropertyTextUpdate.merge(nodePropertyTextEnter);

        // ノードの右下のテキストの内容の描画
        const nodePropertyTextTspanUpdate = nodePropertyText
            .selectAll<SVGTSpanElement, [string, string]>("tspan")
            .data((node) => node);
        const nodePropertyTextTspanEnter = nodePropertyTextTspanUpdate
            .enter()
            .append("tspan")
            .attr("x", 0)
            .attr("dy", "1.2rem");
        nodePropertyTextTspanUpdate.merge(nodePropertyTextTspanEnter).text((prop) => prop);

        nodeGroupEnter.call(
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

        this.#simulation.alphaTarget(0.3).restart();
    }

    setNodeColor(nodeId: NodeId, color: string) {
        this.#nodeRoot
            .selectAll<SVGGElement, Node>("g")
            .filter(({ node }) => node.nodeId.equals(nodeId))
            .select("circle")
            .style("fill", color);
    }

    onClickNode(callback: (node: PartialNode) => void): CancelListening {
        return this.#onClickNode.listen(callback);
    }

    onClickOutsideNode(callback: () => void): CancelListening {
        return this.#onClickOutsideNode.listen(callback);
    }

    onDispose() {
        this.#simulation.stop();
        this.#root.remove();
    }
}

export interface Props {
    rootRef: React.RefObject<HTMLElement>;
}

export const useGraphRenderer = (props: Props) => {
    const { rootRef } = props;

    const rendererRef = useRef<Renderer>();
    useEffect(() => {
        rendererRef.current = new Renderer({
            parent: rootRef.current!,
            nodeRadius: 30,
        });

        return () => {
            rendererRef.current?.onDispose();
        };
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [rootRef]);

    return { rendererRef };
};
