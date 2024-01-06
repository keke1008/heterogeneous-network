import { EchoClient } from "@core/apps/echo";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { useContext, useEffect, useState } from "react";
import { ActionButton, ActionResult } from "../ActionTemplates";
import { TextField } from "@mui/material";

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

const Console: React.FC<{ client: EchoClient }> = ({ client }) => {
    const [messages, setMessages] = useState<string[]>([]);
    useEffect(() => {
        client.onReceive((message) => setMessages((messages) => [...messages, message]));
    }, [client]);

    return (
        <div>
            {messages.map((message) => (
                <div>{message}</div>
            ))}
        </div>
    );
};

const Input: React.FC<{ client: EchoClient }> = ({ client }) => {
    const [input, setInput] = useState<string>("");
    const handleSend = async (): Promise<ActionResult> => {
        const result = await client.send(input);
        if (result.isOk()) {
            return { type: "success" };
        } else {
            return { type: "failure", reason: `${result.unwrapErr()}` };
        }
    };

    return (
        <>
            <TextField type="text" value={input} onChange={(e) => setInput(e.target.value)} />
            <ActionButton onClick={handleSend}>Send</ActionButton>
        </>
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

    if (state.state === "closed") {
        return <Connect onOpen={(client) => setState({ state: "open", client })} />;
    } else {
        return (
            <>
                <Console client={state.client} />
                <Input client={state.client} />
                <Close client={state.client} onClose={() => setState({ state: "closed" })} />
            </>
        );
    }
};
