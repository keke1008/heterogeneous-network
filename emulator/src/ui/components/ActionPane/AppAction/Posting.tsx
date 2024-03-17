import { POSTING_PORT, PostingPacket } from "@core/apps/posting";
import { BufferWriter } from "@core/net";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { Stack, TextField } from "@mui/material";
import { useContext, useState } from "react";
import { ActionButton } from "../ActionTemplates";
import { ActionContext } from "@emulator/ui/contexts/actionContext";

export const PostingApp: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);
    const [message, setMessage] = useState("");

    const handleSubmit = async () => {
        const tunnel = net.tunnel();
        const socket = tunnel.openDynamicPort({ destination: target, destinationPortId: POSTING_PORT }).unwrap();

        const packet = new PostingPacket({ content: message });
        const buffer = BufferWriter.serialize(PostingPacket.serdeable.serializer(packet)).unwrap();

        const result = await socket.send(buffer);
        if (result.isOk()) {
            return { type: "success" } as const;
        } else {
            return { type: "failure", reason: `${result.unwrapErr()}` } as const;
        }
    };

    return (
        <Stack spacing={2}>
            <TextField required label="Message" value={message} onChange={(e) => setMessage(e.target.value)} />
            <ActionButton onClick={handleSubmit}>Post</ActionButton>
        </Stack>
    );
};
