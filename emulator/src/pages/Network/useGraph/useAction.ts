import { NodeId } from "@core/net";
import { useState } from "react";

export interface NodeActionEntry {
    type: "action";
    label: string;
    action: () => void;
}

export interface NodeInfoEntry {
    type: "info";
    label: string;
    value: string;
}

export interface NodeTooltipData {
    nodeId: NodeId;
    x: number;
    y: number;
    entries: (NodeInfoEntry | NodeActionEntry)[];
}

export interface NodeClickEvent {
    nodeId: NodeId;
    x: number;
    y: number;
}

export interface NodeBlurEvent {
    nodeId: NodeId;
}

export const useAction = () => {
    const [nodeTooltip, setNodeTooltip] = useState<NodeTooltipData>();

    const handleClickNode = (event: NodeClickEvent) => {
        const { nodeId, x, y } = event;
        if (nodeTooltip?.nodeId.equals(nodeId) !== true) {
            setNodeTooltip({
                nodeId,
                x,
                y,
                entries: [
                    {
                        type: "info",
                        label: "Node ID",
                        value: nodeId.toString(),
                    },
                    {
                        type: "action",
                        label: "Blink",
                        action: () => {
                            console.log("Blink");
                        },
                    },
                ],
            });
        }
    };

    const handleClickOutsideNode = () => {
        setNodeTooltip(undefined);
    };

    return { nodeTooltip, handleClickNode, handleClickOutsideNode };
};
