import { Address, MediaPortNumber, RpcResult, RpcStatus, SerialAddress } from "@core/net";
import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { ZodSchemaInput } from "@emulator/ui/components/Input";
import { ActionRpcForm } from "../../ActionTemplates";

export const SetSerialAddress: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [mediaPort, setMediaPort] = useState<MediaPortNumber>();
    const [address, setAddress] = useState<SerialAddress>();

    const setSerialAddress = async (): Promise<RpcResult<void>> => {
        if (mediaPort === undefined || address === undefined) {
            return { status: RpcStatus.BadArgument };
        }

        return net.rpc().requestSetSeriaAddress(target, mediaPort, new Address(address));
    };

    return (
        <ActionRpcForm onSubmit={setSerialAddress} submitButtonText="Set serial address">
            <ZodSchemaInput<MediaPortNumber | undefined>
                schema={MediaPortNumber.schema}
                label="Media Port"
                onValue={(value) => setMediaPort(value)}
                fullWidth
            />
            <ZodSchemaInput<SerialAddress | undefined>
                schema={SerialAddress.schema}
                label="Address"
                onValue={(value) => setAddress(value)}
                fullWidth
            />
        </ActionRpcForm>
    );
};
