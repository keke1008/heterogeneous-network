import { Address, Cost, NodeId, RpcResult, RpcStatus } from "@core/net";
import { Action, ActionRpcButton, ActionGroup, ActionParameter } from "./ActionTemplates";
import { useContext, useState } from "react";
import { AddressInput, NodeIdInput, CostInput } from "../Input";
import { NetContext } from "@emulator/ui/contexts/netContext";

interface Props {
    targetId: NodeId;
}

const SendHello: React.FC<Props> = ({ targetId }) => {
    const net = useContext(NetContext);
    const [address, setAddress] = useState<Address | undefined>();
    const [cost, setCost] = useState<Cost | undefined>();
    const sendHello = async (): Promise<RpcResult<unknown>> => {
        if (address && cost) {
            return net.rpc().requestSendHello(targetId, address, cost);
        } else {
            return { status: RpcStatus.BadArgument };
        }
    };

    return (
        <Action>
            <ActionRpcButton onClick={sendHello}>Send Hello</ActionRpcButton>
            <ActionParameter>
                <AddressInput label="target address" onChange={setAddress} />
            </ActionParameter>
            <ActionParameter>
                <CostInput label="cost" onChange={setCost} />
            </ActionParameter>
        </Action>
    );
};

const SendGoodbye: React.FC<Props> = ({ targetId }) => {
    const net = useContext(NetContext);
    const [nodeId, setNodeId] = useState<NodeId | undefined>();
    const sendGoodbye = async (): Promise<RpcResult<unknown>> => {
        if (nodeId) {
            return net.rpc().requestSendGoodbye(targetId, nodeId);
        } else {
            return { status: RpcStatus.BadArgument };
        }
    };

    return (
        <Action>
            <ActionRpcButton onClick={sendGoodbye}>Send Goodbye</ActionRpcButton>
            <ActionParameter>
                <NodeIdInput label="target address" onChange={setNodeId} />
            </ActionParameter>
        </Action>
    );
};

export const Neighbor: React.FC<Props> = ({ targetId }) => {
    return (
        <ActionGroup name="Neighbor">
            <SendHello targetId={targetId} />
            <SendGoodbye targetId={targetId} />
        </ActionGroup>
    );
};
