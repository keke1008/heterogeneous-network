import { fetchAiImageUrl } from "@core/apps/ai-image";
import { AiImageMessageData, Message, TextMessageData } from "@core/apps/chat";
import { spawn } from "@core/async";
import { Box, CircularProgress, Stack } from "@mui/material";
import { useContext, useEffect, useState } from "react";
import { ChatConfigContext } from "./Context";
import { match } from "ts-pattern";

const TextMessage: React.FC<{ data: TextMessageData }> = ({ data }) => {
    return data.text;
};

const SelfAiImageMessage: React.FC<{ data: AiImageMessageData }> = ({ data }) => {
    return data.prompt;
};

interface PeerAiImageMessageProps {
    data: AiImageMessageData;
}

const PeerAiImageMessage: React.FC<PeerAiImageMessageProps> = ({ data }) => {
    const { aiImageServerAddress } = useContext(ChatConfigContext).config;

    const [imageUrl, setImageUrl] = useState<string>();
    useEffect(() => {
        data.imageUrl?.then(setImageUrl);
    }, [data.imageUrl]);

    const [loading, setLoading] = useState(true);
    useEffect(() => {
        if (aiImageServerAddress === undefined) {
            setLoading(false);
            return;
        }

        setLoading(true);
        const handle = spawn(async (signal) => {
            data.imageUrl ??= fetchAiImageUrl(aiImageServerAddress, data.prompt).then((res) => {
                return res.isOk() ? res.unwrap() : undefined;
            });

            const imageUrl = await data.imageUrl;
            if (!signal.aborted) {
                setImageUrl(imageUrl);
                setLoading(false);
            }
        });

        return () => handle.cancel();
    }, [aiImageServerAddress, data, data.imageUrl, data.prompt]);

    return imageUrl ? (
        <Box maxWidth="50vh">
            <img src={imageUrl} style={{ maxWidth: "100%" }} />
        </Box>
    ) : (
        <Stack>
            {data.prompt}
            {loading && <CircularProgress />}
        </Stack>
    );
};

interface ChatMessageProps {
    message: Message;
}

export const ChatMessage: React.FC<ChatMessageProps> = ({ message }) => {
    return match(message)
        .with({ data: { type: "text" } }, ({ data }) => <TextMessage data={data} />)
        .with({ data: { type: "ai-image" }, sender: "self" }, ({ data }) => <SelfAiImageMessage data={data} />)
        .with({ data: { type: "ai-image" }, sender: "peer" }, ({ data }) => <PeerAiImageMessage data={data} />)
        .exhaustive();
};
