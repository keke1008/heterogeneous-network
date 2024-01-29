import { MediaPortNumber, RpcResult, RpcStatus } from "@core/net";
import { useState } from "react";
import { useContext } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ZodSchemaInput } from "@emulator/ui/components/Input";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { ActionRpcForm } from "../../ActionTemplates";

export const CloseServer: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [mediaPort, setMediaPort] = useState<MediaPortNumber>();

    const startServer = async (): Promise<RpcResult<void>> => {
        if (mediaPort === undefined) {
            return { status: RpcStatus.BadArgument };
        }
        return net.rpc().requestCloseServer(target, mediaPort);
    };

    return (
        <ActionRpcForm onSubmit={startServer} submitButtonText="Close server">
            <ZodSchemaInput<MediaPortNumber | undefined>
                schema={MediaPortNumber.schema}
                label="Media Port"
                fullWidth
                onValue={(value) => setMediaPort(value)}
            />
        </ActionRpcForm>
    );
};
