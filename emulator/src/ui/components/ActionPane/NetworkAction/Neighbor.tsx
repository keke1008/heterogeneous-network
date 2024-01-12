import { Address, Cost, MediaPortNumber, RpcResult, RpcStatus } from "@core/net";
import { ActionGroup, ActionRpcDialog } from "../ActionTemplates";
import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { AddressInput, ZodSchemaInput } from "@emulator/ui/components/Input";

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

export const Neighbor: React.FC = () => {
    return (
        <ActionGroup name="Neighbor">
            <SendHello />
        </ActionGroup>
    );
};
