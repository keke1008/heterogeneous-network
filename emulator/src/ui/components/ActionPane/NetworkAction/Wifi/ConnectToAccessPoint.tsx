import { MediaPortNumber, RpcResult, RpcStatus } from "@core/net";
import { TextField } from "@mui/material";
import { useState } from "react";
import { useContext } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ZodSchemaInput } from "@emulator/ui/components/Input";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { ActionRpcForm } from "../../ActionTemplates";

export const ConnectToAccessPoint: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [ssid, setSsid] = useState<string>("");
    const [password, setPassword] = useState<string>("");
    const [mediaPort, setMediaPort] = useState<MediaPortNumber>();
    const connectToAccessPoint = async (): Promise<RpcResult<void>> => {
        if (mediaPort === undefined) {
            return { status: RpcStatus.BadArgument };
        }

        const encoder = new TextEncoder();
        return net.rpc().requestConnectToAccessPoint(target, encoder.encode(ssid), encoder.encode(password), mediaPort);
    };

    return (
        <ActionRpcForm onSubmit={connectToAccessPoint} submitButtonText="Connect to access point">
            <TextField
                size="small"
                label="SSID"
                variant="outlined"
                fullWidth
                onChange={(e) => setSsid(e.target.value)}
            />
            <TextField
                size="small"
                label="Password"
                variant="outlined"
                type="password"
                fullWidth
                onChange={(e) => setPassword(e.target.value)}
            />
            <ZodSchemaInput<MediaPortNumber | undefined>
                schema={MediaPortNumber.schema}
                label="Media Port"
                fullWidth
                onValue={(value) => setMediaPort(value)}
            />
        </ActionRpcForm>
    );
};
