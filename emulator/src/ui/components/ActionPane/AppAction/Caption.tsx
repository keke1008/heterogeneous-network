import { CaptionClient, CaptionStatus, ClearCaption, ServerInfo, ShowCaption } from "@core/apps/caption";
import { FormControl, Grid, Input, InputLabel, Tab } from "@mui/material";
import { useContext, useEffect, useState } from "react";
import { ActionButton, ActionResult } from "../ActionTemplates";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { TabContext, TabList, TabPanel } from "@mui/lab";

interface ServerInputProps {
    onChange: (value: ServerInfo) => void;
}

const ServerInput: React.FC<ServerInputProps> = ({ onChange }) => {
    const [address, setAddress] = useState<string>("");
    const [port, setPort] = useState<number>(0);
    useEffect(() => {
        onChange(new ServerInfo({ address, port }));
    }, [address, port, onChange]);

    return (
        <Grid container spacing={2}>
            <Grid item xs={6}>
                <FormControl>
                    <InputLabel htmlFor="address">Address</InputLabel>
                    <Input id="address" onChange={(e) => setAddress(e.target.value)} />
                </FormControl>
            </Grid>
            <Grid item xs={6}>
                <FormControl>
                    <InputLabel htmlFor="port">Port</InputLabel>
                    <Input id="port" type="number" onChange={(e) => setPort(parseInt(e.target.value))} />
                </FormControl>
            </Grid>
        </Grid>
    );
};

interface ShowCaptionInputProps {
    client: CaptionClient;
    server: ServerInfo;
}

const ShowCaptionInput: React.FC<ShowCaptionInputProps> = ({ client, server }) => {
    const [x, setX] = useState<number>(0);
    const [y, setY] = useState<number>(0);
    const [fontSize, setFontSize] = useState<number>(0);
    const [text, setText] = useState<string>("");

    const handleClick = async (): Promise<ActionResult> => {
        const caption = new ShowCaption({ x, y, fontSize, text, server });
        const result = await client.send(caption);
        return result.status === CaptionStatus.Success ? { type: "success" } : { type: "failure", reason: "Failure" };
    };

    return (
        <Grid container spacing={2}>
            <Grid item xs={4}>
                <FormControl>
                    <InputLabel htmlFor="x">X</InputLabel>
                    <Input id="x" type="number" onChange={(e) => setX(parseInt(e.target.value))} />
                </FormControl>
            </Grid>
            <Grid item xs={4}>
                <FormControl>
                    <InputLabel htmlFor="y">Y</InputLabel>
                    <Input type="number" onChange={(e) => setY(parseInt(e.target.value))} />
                </FormControl>
            </Grid>
            <Grid item xs={4}>
                <FormControl>
                    <InputLabel htmlFor="fontSize">Font size</InputLabel>
                    <Input type="number" onChange={(e) => setFontSize(parseInt(e.target.value))} />
                </FormControl>
            </Grid>

            <Grid item xs={12}>
                <FormControl>
                    <InputLabel htmlFor="text">Text</InputLabel>
                    <Input onChange={(e) => setText(e.target.value)} />
                </FormControl>
            </Grid>
            <Grid item xs={12}>
                <ActionButton onClick={handleClick}>Show caption</ActionButton>
            </Grid>
        </Grid>
    );
};

interface ClearCaptionInputProps {
    client: CaptionClient;
    server: ServerInfo;
}

const ClearCaptionInput: React.FC<ClearCaptionInputProps> = ({ client, server }) => {
    const handleClick = async (): Promise<ActionResult> => {
        const result = await client.send(new ClearCaption({ server }));
        return result.status === CaptionStatus.Success ? { type: "success" } : { type: "failure", reason: "Failure" };
    };

    return <ActionButton onClick={handleClick}>Clear caption</ActionButton>;
};

interface ConnectionProps {
    onOpen: (client: CaptionClient) => void;
}

const Connection: React.FC<ConnectionProps> = ({ onOpen }) => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const handleClick = async (): Promise<ActionResult> => {
        const client = await CaptionClient.connect({ trustedService: net.trusted(), destination: target });
        if (client.isOk()) {
            onOpen(client.unwrap());
            return { type: "success" };
        } else {
            return { type: "failure", reason: `${client.unwrapErr()}` };
        }
    };

    return <ActionButton onClick={handleClick}>Connect</ActionButton>;
};

export const Caption: React.FC = () => {
    const [client, setClient] = useState<CaptionClient>();

    const [tab, setTab] = useState<"show" | "clear">("show");
    const [server, setServer] = useState<ServerInfo>(new ServerInfo({ address: "", port: 0 }));

    if (client === undefined) {
        return <Connection onOpen={setClient} />;
    } else {
        return (
            <>
                <ServerInput onChange={setServer} />
                <TabContext value={tab}>
                    <TabList onChange={(_, v) => setTab(v)}>
                        <Tab value="show" label="Show" />
                        <Tab value="clear" label="Clear" />
                    </TabList>
                    <TabPanel value="show">
                        <ShowCaptionInput client={client} server={server} />
                    </TabPanel>
                    <TabPanel value="clear">
                        <ClearCaptionInput client={client} server={server} />
                    </TabPanel>
                </TabContext>
            </>
        );
    }
};