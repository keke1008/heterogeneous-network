import { BlinkOperation, NodeId, RpcStatus } from "@core/net";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { useContext, useState } from "react";

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
    const net = useContext(NetContext);
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
                        action: async () => {
                            const result = await net.rpc().requestBlink(nodeId, BlinkOperation.Blink);
                            console.log(RpcStatus[result.status]);
                        },
                    },
                    {
                        type: "action",
                        label: "Blink stop",
                        action: async () => {
                            const result = await net.rpc().requestBlink(nodeId, BlinkOperation.Stop);
                            console.log(RpcStatus[result.status]);
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
