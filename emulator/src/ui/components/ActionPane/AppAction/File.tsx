import { FileClient } from "@core/apps/file";
import { ConnectionButton } from "./ConnectionButton";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { useContext, useState } from "react";
import { Stack, TextField } from "@mui/material";
import { ActionButton } from "../ActionTemplates";

interface Props {
    client: FileClient;
}

const SendFile: React.FC<Props> = ({ client }) => {
    const [files, setFiles] = useState<FileList | null>(null);

    const sendFile = async () => {
        if (files === null || files.length === 0) {
            return { type: "failure", reason: "No file selected" } as const;
        }

        const file = files[0];
        const data = new Uint8Array(await file.arrayBuffer());
        await client.sendFile({ filename: file.name, data });
        return { type: "success" } as const;
    };

    return (
        <Stack spacing={2}>
            <TextField
                required
                type="file"
                label="File"
                onChange={({ target }) => "files" in target && setFiles(target.files)}
            />
            <ActionButton onClick={sendFile}>Send</ActionButton>
        </Stack>
    );
};

export const FileAction: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const connect = async () => {
        const client = await FileClient.connect({ streamService: net.stream(), destination: target });
        return client.isOk()
            ? ({ type: "success", client: client.unwrap() } as const)
            : ({ type: "failure", reason: `${client.unwrapErr()}` } as const);
    };
    return <ConnectionButton connect={connect}>{(client) => <SendFile client={client} />}</ConnectionButton>;
};
