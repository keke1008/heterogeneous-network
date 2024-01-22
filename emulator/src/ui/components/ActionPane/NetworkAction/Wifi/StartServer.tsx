import { MediaPortNumber, RpcResult, RpcStatus } from "@core/net";
import { TextField } from "@mui/material";
import { useState } from "react";
import { useContext } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ZodSchemaInput } from "@emulator/ui/components/Input";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { ActionRpcForm } from "../../ActionTemplates";

export const StartServer: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [port, setPort] = useState<number>(0);
    const [mediaPort, setMediaPort] = useState<MediaPortNumber>();

    const startServer = async (): Promise<RpcResult<void>> => {
        if (mediaPort === undefined) {
            return { status: RpcStatus.BadArgument };
        }
        return net.rpc().requestStartServer(target, port, mediaPort);
    };

    return (
        <ActionRpcForm onSubmit={startServer} submitButtonText="Start server">
            <TextField type="number" label="port" fullWidth onChange={(e) => setPort(parseInt(e.target.value))} />
            <ZodSchemaInput<MediaPortNumber | undefined>
                schema={MediaPortNumber.schema}
                label="Media Port"
                fullWidth
                onValue={(value) => setMediaPort(value)}
            />
        </ActionRpcForm>
    );
};
