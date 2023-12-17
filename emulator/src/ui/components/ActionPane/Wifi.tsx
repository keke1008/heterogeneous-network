import { Destination } from "@core/net";
import { ActionGroup } from "./ActionTemplates";
import { TextField } from "@mui/material";
import { useState } from "react";
import { useContext } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionRpcDialog } from "./ActionTemplates/ActionDialog";

interface Props {
    targetNode: Destination;
}

const ConnectToAccessPoint: React.FC<Props> = ({ targetNode }) => {
    const net = useContext(NetContext);
    const connectToAccessPoint = () => {
        const encoder = new TextEncoder();
        return net.rpc().requestConnectToAccessPoint(targetNode, encoder.encode(ssid), encoder.encode(password));
    };
    const [ssid, setSsid] = useState<string>("");
    const [password, setPassword] = useState<string>("");

    return (
        <ActionRpcDialog name="Connect to Access Point" onSubmit={connectToAccessPoint}>
            <TextField size="small" label="SSID" variant="outlined" onChange={(e) => setSsid(e.target.value)} />
            <TextField size="small" label="Password" variant="outlined" onChange={(e) => setPassword(e.target.value)} />
        </ActionRpcDialog>
    );
};

const StartServer: React.FC<Props> = ({ targetNode }) => {
    const net = useContext(NetContext);
    const [port, setPort] = useState<number>(0);
    const startServer = () => net.rpc().requestStartServer(targetNode, port);
    return (
        <ActionRpcDialog name="Start Server" onSubmit={startServer}>
            <TextField size="small" type="number" label="port" onChange={(e) => setPort(parseInt(e.target.value))} />
        </ActionRpcDialog>
    );
};

export const Wifi: React.FC<Props> = ({ targetNode }) => {
    return (
        <ActionGroup name="Wifi">
            <ConnectToAccessPoint targetNode={targetNode} />
            <StartServer targetNode={targetNode} />
        </ActionGroup>
    );
};
