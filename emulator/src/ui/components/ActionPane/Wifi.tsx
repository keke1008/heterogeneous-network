import { NodeId } from "@core/net";
import { Action, ActionButton, ActionGroup, ActionParameter } from "./ActionTemplates";
import { TextField } from "@mui/material";
import { useState } from "react";
import { useContext } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";

interface Props {
    targetId: NodeId;
}

const ConnectToAccessPoint: React.FC<Props> = ({ targetId }) => {
    const net = useContext(NetContext);
    const connectToAccessPoint = () => {
        const encoder = new TextEncoder();
        return net.rpc().requestConnectToAccessPoint(targetId, encoder.encode(ssid), encoder.encode(password));
    };
    const [ssid, setSsid] = useState<string>("");
    const [password, setPassword] = useState<string>("");

    return (
        <Action>
            <ActionButton onClick={connectToAccessPoint}>Connect To AP</ActionButton>
            <ActionParameter>
                <TextField size="small" label="SSID" variant="outlined" onChange={(e) => setSsid(e.target.value)} />
            </ActionParameter>
            <ActionParameter>
                <TextField
                    size="small"
                    label="Password"
                    variant="outlined"
                    onChange={(e) => setPassword(e.target.value)}
                />
            </ActionParameter>
        </Action>
    );
};

const StartServer: React.FC<Props> = ({ targetId }) => {
    const net = useContext(NetContext);
    const [port, setPort] = useState<number>(0);
    const startServer = () => net.rpc().requestStartServer(targetId, port);
    return (
        <Action>
            <ActionButton onClick={startServer}>Start Server</ActionButton>
            <ActionParameter>
                <TextField
                    size="small"
                    type="number"
                    label="port"
                    onChange={(e) => setPort(parseInt(e.target.value))}
                />
            </ActionParameter>
        </Action>
    );
};

export const Wifi: React.FC<Props> = ({ targetId }) => {
    return (
        <ActionGroup name="Wifi">
            <ConnectToAccessPoint targetId={targetId} />
            <StartServer targetId={targetId} />
        </ActionGroup>
    );
};
