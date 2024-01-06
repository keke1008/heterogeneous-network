import { MediaPortNumber, RpcResult, RpcStatus, SerialAddress } from "@core/net";
import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionGroup } from "./ActionTemplates";
import { ActionRpcDialog } from "./ActionTemplates/ActionDialog";
import { ZodSchemaInput } from "../Input";
import { ActionContext } from "@emulator/ui/contexts/actionContext";

export const Serial: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [mediaPort, setMediaPort] = useState<MediaPortNumber>();
    const [address, setAddress] = useState<SerialAddress>();

    const setSerialAddress = async (): Promise<RpcResult<void>> => {
        if (mediaPort === undefined || address === undefined) {
            return { status: RpcStatus.BadArgument };
        }

        return net.rpc().requestSetSeriaAddress(target, mediaPort, address);
    };

    return (
        <ActionGroup name="Serial">
            <ActionRpcDialog name="Set Serial Address" onSubmit={setSerialAddress}>
                <ZodSchemaInput<MediaPortNumber | undefined>
                    schema={MediaPortNumber.schema}
                    label="Media Port"
                    onValue={(value) => setMediaPort(value)}
                />
                <ZodSchemaInput<SerialAddress | undefined>
                    schema={SerialAddress.schema}
                    label="Address"
                    onValue={(value) => setAddress(value)}
                />
            </ActionRpcDialog>
        </ActionGroup>
    );
};
