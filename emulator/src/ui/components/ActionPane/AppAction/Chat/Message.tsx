import { fetchAiImageUrl } from "@core/apps/ai-image";
import { AiImageMessageData, Message, TextMessageData } from "@core/apps/chat";
import { spawn } from "@core/async";
import { Card, CardContent, CircularProgress, Stack } from "@mui/material";
import { useEffect, useState } from "react";

const MessageContainer: React.FC<{ children: React.ReactNode }> = ({ children }) => {
    return (
        <Card>
            <CardContent>{children}</CardContent>
        </Card>
    );
};

const TextMessage: React.FC<{ data: TextMessageData }> = ({ data }) => {
    return <MessageContainer>{data.text}</MessageContainer>;
};

const AiImagePromptMessage: React.FC<{ prompt: string }> = ({ prompt }) => {
    return (
        <MessageContainer>
            <Stack>
                {prompt}
                <CircularProgress />
            </Stack>
        </MessageContainer>
    );
};

const AiImageImageMessage: React.FC<{ imageUrl: string }> = ({ imageUrl }) => {
    return (
        <MessageContainer>
            <img src={imageUrl} />
        </MessageContainer>
    );
};

interface AiImageMessageProps {
    aiImageServerAddress?: string;
    data: AiImageMessageData;
}

const AiImageMessage: React.FC<AiImageMessageProps> = ({ aiImageServerAddress, data }) => {
    const [imageUrl, setImageUrl] = useState<string>();
    useEffect(() => {
        data.imageUrl?.then(setImageUrl);
    }, [data.imageUrl]);

    useEffect(() => {
        if (aiImageServerAddress === undefined) {
            return;
        }

        const handle = spawn(async (signal) => {
            data.imageUrl ??= fetchAiImageUrl(aiImageServerAddress, data.prompt).then((res) => {
                return res.isOk() ? res.unwrap() : undefined;
            });

            const imageUrl = await data.imageUrl;
            !signal.aborted && setImageUrl(imageUrl);
        });

        return () => handle.cancel();
    }, [aiImageServerAddress, data, data.imageUrl, data.prompt]);

    return imageUrl ? <AiImageImageMessage imageUrl={imageUrl} /> : <AiImagePromptMessage prompt={data.prompt} />;
};

interface ChatMessageProps {
    message: Message;
    aiImageServerAddress?: string;
}

export const ChatMessage: React.FC<ChatMessageProps> = ({ message, aiImageServerAddress }) => {
    return message.data.type === "text" ? (
        <TextMessage data={message.data} />
    ) : (
        <AiImageMessage data={message.data} aiImageServerAddress={aiImageServerAddress} />
    );
};
