import { useRef } from "react";
import { useGraph } from "./useGraph";
import { NodeInfoEntry, NodeActionEntry, NodeTooltipData } from "./useGraph/useAction";

const NodeInfo = ({ entry: { label, value } }: { entry: NodeInfoEntry }) => {
    return (
        <>
            <td>{label}:</td>
            <td>{value}</td>
        </>
    );
};

const NodeAction = ({ entry: { label, action } }: { entry: NodeActionEntry }) => {
    return (
        <td colSpan={2}>
            <button onClick={action}>{label}</button>
        </td>
    );
};

const NodeEntry = ({ entry }: { entry: NodeInfoEntry | NodeActionEntry }) => {
    const child = entry.type === "info" ? <NodeInfo entry={entry} /> : <NodeAction entry={entry} />;
    return <tr>{child}</tr>;
};

const NodeTooltip = (data: NodeTooltipData) => {
    return (
        <table
            style={{
                position: "absolute",
                left: data.x,
                top: data.y,
                border: "1px solid black",
                backgroundColor: "white",
            }}
        >
            <tbody>
                {data.entries.map((entry) => (
                    <NodeEntry key={entry.label} entry={entry} />
                ))}
            </tbody>
        </table>
    );
};

export const Graph = () => {
    const rootRef = useRef<HTMLDivElement>(null);
    const { nodeTooltip } = useGraph(rootRef);

    return (
        <>
            <div ref={rootRef}></div>
            {nodeTooltip && <NodeTooltip {...nodeTooltip} />}
        </>
    );
};
