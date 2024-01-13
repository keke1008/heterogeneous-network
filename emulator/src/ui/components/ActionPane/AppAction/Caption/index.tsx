import { CaptionClient, ServerInfo } from "@core/apps/caption";
import { Stack, Tab } from "@mui/material";
import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { TabContext, TabList, TabPanel } from "@mui/lab";
import { ConnectionButton } from "../ConnectionButton";
import { ShowCaptionInput } from "./ShowCaptionInput";
import { ClearCaptionInput } from "./ClearCaptionInput";
import { ServerInput } from "./ServerInput";

export const CaptionInput: React.FC<{ client: CaptionClient }> = ({ client }) => {
    const [tab, setTab] = useState<"show" | "clear">("show");
    const [server, setServer] = useState<ServerInfo>(new ServerInfo({ address: "", port: 0 }));

    return (
        <Stack spacing={2}>
            <ServerInput onChange={setServer} />
            <TabContext value={tab}>
                <TabList onChange={(_, v) => setTab(v)} sx={{ width: "100%" }}>
                    <Tab value="show" label="Show" sx={{ width: "50%" }} />
                    <Tab value="clear" label="Clear" sx={{ width: "50%" }} />
                </TabList>
                <TabPanel value="show">
                    <ShowCaptionInput client={client} server={server} />
                </TabPanel>
                <TabPanel value="clear">
                    <ClearCaptionInput client={client} server={server} />
                </TabPanel>
            </TabContext>
        </Stack>
    );
};

export const Caption: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const connect = async () => {
        const client = await CaptionClient.connect({ trustedService: net.trusted(), destination: target });
        return client.isOk()
            ? ({ type: "success", client: client.unwrap() } as const)
            : ({ type: "failure", reason: `${client.unwrapErr()}` } as const);
    };

    return <ConnectionButton connect={connect}>{(client) => <CaptionInput client={client} />}</ConnectionButton>;
};
