import { Address, Cost, MediaPortNumber, NodeId, RpcResult, RpcStatus } from "@core/net";
import { ActionGroup } from "./ActionTemplates";
import { useContext, useState } from "react";
import { AddressInput, NodeIdInput } from "../Input";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ZodSchemaInput } from "../Input/ZodSchemaInput";
import { ActionRpcDialog } from "./ActionTemplates/ActionDialog";
import { ActionContext } from "@emulator/ui/contexts/actionContext";

const SendHello: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [address, setAddress] = useState<Address | undefined>();
    const [cost, setCost] = useState<Cost | undefined>();
    const [mediaPort, setMediaPort] = useState<MediaPortNumber | undefined>();
    const sendHello = async (): Promise<RpcResult<unknown>> => {
        if (address && cost) {
            return net.rpc().requestSendHello(target, address, cost, mediaPort);
        } else {
            return { status: RpcStatus.BadArgument };
        }
    };

    return (
        <ActionRpcDialog name="Send Hello" onSubmit={sendHello}>
            <AddressInput label="targetNode address" stringValue="127.0.0.1:10001" onValue={setAddress} />
            <ZodSchemaInput<Cost> label="link cost" schema={Cost.schema} onValue={setCost} stringValue="0" />
            <ZodSchemaInput<MediaPortNumber>
                label="media port"
                schema={MediaPortNumber.schema}
                onValue={setMediaPort}
                allowEmpty={undefined}
            />
        </ActionRpcDialog>
    );
};

const SendGoodbye: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [nodeId, setNodeId] = useState<NodeId | undefined>();
    const sendGoodbye = async (): Promise<RpcResult<unknown>> => {
        if (nodeId) {
            return net.rpc().requestSendGoodbye(target, nodeId);
        } else {
            return { status: RpcStatus.BadArgument };
        }
    };

    return (
        <ActionRpcDialog name="Send Goodbye" onSubmit={sendGoodbye}>
            <NodeIdInput label="targetNode address" onValue={setNodeId} />
        </ActionRpcDialog>
    );
};

export const Neighbor: React.FC = () => {
    return (
        <ActionGroup name="Neighbor">
            <SendHello />
            <SendGoodbye />
        </ActionGroup>
    );
};
