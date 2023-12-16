import { Address, AddressType, Cost, MediaPortNumber, NodeId, RpcResult, RpcStatus } from "@core/net";
import { Action, ActionRpcButton, ActionGroup, ActionParameter } from "./ActionTemplates";
import { useContext, useState } from "react";
import { AddressInput, NodeIdInput } from "../Input";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ZodSchemaInput } from "../Input/ZodSchemaInput";

interface Props {
    targetId: NodeId;
}

const SendHello: React.FC<Props> = ({ targetId }) => {
    const net = useContext(NetContext);
    const [address, setAddress] = useState<Address | undefined>();
    const [cost, setCost] = useState<Cost | undefined>();
    const [mediaPort, setMediaPort] = useState<MediaPortNumber | undefined>();
    const sendHello = async (): Promise<RpcResult<unknown>> => {
        if (address && cost) {
            return net.rpc().requestSendHello(targetId, address, cost, mediaPort);
        } else {
            return { status: RpcStatus.BadArgument };
        }
    };

    return (
        <Action>
            <ActionRpcButton onClick={sendHello}>Send Hello</ActionRpcButton>
            <ActionParameter>
                <AddressInput
                    label="target address"
                    initialType={AddressType.Udp}
                    stringValue="127.0.0.1:10001"
                    onChange={setAddress}
                />
            </ActionParameter>
            <ActionParameter>
                <ZodSchemaInput<Cost> label="link cost" schema={Cost.schema} onValue={setCost} stringValue="0" />
            </ActionParameter>
            <ActionParameter>
                <ZodSchemaInput<MediaPortNumber>
                    label="media port"
                    schema={MediaPortNumber.schema}
                    onValue={setMediaPort}
                    allowEmpty={undefined}
                />
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
