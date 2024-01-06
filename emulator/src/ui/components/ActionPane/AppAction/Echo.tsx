import { EchoClient } from "@core/apps/echo";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { useContext, useEffect, useState } from "react";
import { ActionButton, ActionResult } from "../ActionTemplates";
import { Grid, List, ListItem, ListItemText, TextField } from "@mui/material";

interface ConnectProps {
    onOpen: (client: EchoClient) => void;
}

const Connect: React.FC<ConnectProps> = ({ onOpen }) => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const handleClick = async (): Promise<ActionResult> => {
        const client = await EchoClient.connect({ trustedService: net.trusted(), destination: target });
        if (client.isOk()) {
            onOpen(client.unwrap());
            return { type: "success" };
        } else {
            return { type: "failure", reason: `${client.unwrapErr()}` };
        }
    };

    return <ActionButton onClick={handleClick}>Connect</ActionButton>;
};

const Input: React.FC<{ client?: EchoClient }> = ({ client }) => {
    const [input, setInput] = useState<string>("");
    let handleSend;
    if (client) {
        handleSend = async (): Promise<ActionResult> => {
            const result = await client.send(input);
            if (result.isOk()) {
                return { type: "success" };
            } else {
                return { type: "failure", reason: `${result.unwrapErr()}` };
            }
        };
    }

    const disabled = client === undefined;

    return (
        <Grid container>
            <Grid item xs={10}>
                <TextField fullWidth disabled={disabled} value={input} onChange={(e) => setInput(e.target.value)} />
            </Grid>
            <Grid item xs={2}>
                <ActionButton onClick={handleSend} buttonProps={{ disabled, sx: { height: "100%" } }}>
                    Send
                </ActionButton>
            </Grid>
        </Grid>
    );
};

interface CloseProps {
    client: EchoClient;
    onClose: () => void;
}

const Close: React.FC<CloseProps> = ({ client, onClose }) => {
    const handleClick = async (): Promise<ActionResult> => {
        await client.close();
        onClose();
        return { type: "success" };
    };

    return <ActionButton onClick={handleClick}>Close</ActionButton>;
};

export const Echo: React.FC = () => {
    type State = { state: "closed" } | { state: "open"; client: EchoClient };
    const [state, setState] = useState<State>({ state: "closed" });

    const [messages, setMessages] = useState<string[]>([]);
    useEffect(() => {
        if (state.state === "open") {
            return state.client.onReceive((message) => {
                setMessages((messages) => (messages.length >= 10 ? messages.slice(1, 10) : messages).concat(message));
            });
        }
    }, [state]);

    return (
        <Grid container spacing={2} direction="column">
            <Grid item>
                {state.state === "closed" ? (
                    <Connect onOpen={(client) => setState({ state: "open", client })} />
                ) : (
                    <Close client={state.client} onClose={() => setState({ state: "closed" })} />
                )}
            </Grid>
            <Grid item xs={12}>
                <List sx={{ border: 1, borderRadius: 1, borderColor: "primary.main" }}>
                    {messages.map((message, i) => (
                        <ListItem key={i}>
                            <ListItemText>{message}</ListItemText>
                        </ListItem>
                    ))}
                </List>
            </Grid>
            <Grid item>
                <Input client={state.state === "closed" ? undefined : state.client} />
            </Grid>
        </Grid>
    );
};
