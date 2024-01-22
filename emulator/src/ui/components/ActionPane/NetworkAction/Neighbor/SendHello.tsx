import { Address, Cost, MediaPortNumber, RpcResult, RpcStatus } from "@core/net";
import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { AddressInput, ZodSchemaInput } from "@emulator/ui/components/Input";
import { ActionRpcForm } from "../../ActionTemplates";

export const SendHello: React.FC = () => {
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
        <ActionRpcForm onSubmit={sendHello} submitButtonText="Send hello">
            <AddressInput
                label="targetNode address"
                stringValue="127.0.0.1:10001"
                onValue={setAddress}
                textProps={{ fullWidth: true }}
            />
            <ZodSchemaInput<Cost> label="link cost" schema={Cost.schema} onValue={setCost} stringValue="0" fullWidth />
            <ZodSchemaInput<MediaPortNumber>
                label="media port"
                schema={MediaPortNumber.schema}
                onValue={setMediaPort}
                allowEmpty={undefined}
                fullWidth
            />
        </ActionRpcForm>
    );
};
