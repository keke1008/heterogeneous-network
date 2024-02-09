import { AiImageClient } from "@core/apps/ai-image";
import { GeneratedImage } from "@emulator/apps/aiImage";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { AppContext } from "@emulator/ui/contexts/appContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ImageList, ImageListItem, Stack, TextField } from "@mui/material";
import { useContext, useEffect, useState } from "react";
import { ConnectionButton } from "../ConnectionButton";
import { ActionButton } from "../../ActionTemplates";

interface Props {
    client: AiImageClient;
}

const SendPrompt: React.FC<Props> = ({ client }) => {
    const [httpServerAddress, setHttpServerAddress] = useState("");
    const [prompt, setPrompt] = useState("");

    const handleSend = async () => {
        const result = await client.send({ prompt, httpServerAddress });
        return result.isOk()
            ? ({ type: "success" } as const)
            : ({ type: "failure", reason: result.unwrapErr() } as const);
    };

    return (
        <Stack spacing={2}>
            <TextField
                required
                label="HTTP Server Address"
                value={httpServerAddress}
                onChange={(e) => setHttpServerAddress(e.target.value)}
            />
            <TextField
                required
                multiline
                minRows={2}
                label="Prompt"
                value={prompt}
                onChange={(e) => setPrompt(e.target.value)}
            />
            <ActionButton onClick={handleSend}>Send</ActionButton>
        </Stack>
    );
};

const ReceivedImages: React.FC = () => {
    const [images, setImages] = useState<GeneratedImage[]>([]);
    const server = useContext(AppContext).aiImageServer();
    useEffect(() => {
        return server.onImageGenerated((image) => setImages((prev) => [image, ...prev]));
    }, [server]);

    return (
        <ImageList cols={2} sx={{ overflowY: "scroll" }}>
            {images.map((image, index) => (
                <ImageListItem key={index}>
                    <img src={image.url} alt={image.prompt} />
                </ImageListItem>
            ))}
        </ImageList>
    );
};

export const AiImage: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const connect = async () => {
        const client = await AiImageClient.connect({ trustedService: net.trusted(), destination: target });
        return client.isOk()
            ? ({ type: "success", client: client.unwrap() } as const)
            : ({ type: "failure", reason: `${client.unwrapErr()}` } as const);
    };

    return (
        <Stack width="100%" height="100%">
            <ConnectionButton connect={connect}>
                {(client) => (
                    <Stack spacing={2}>
                        <SendPrompt client={client} />
                    </Stack>
                )}
            </ConnectionButton>
            <ReceivedImages />
        </Stack>
    );
};
