import { Grid, MenuItem, Select, SelectChangeEvent, TextField } from "@mui/material";
import { useContext, useState } from "react";
import { ChatRoomContext } from "./Context";
import { Send } from "@mui/icons-material";
import { P, match } from "ts-pattern";
import { LoadingButton } from "@mui/lab";

type InputContent = { type: "text"; text?: string } | { type: "ai-image"; prompt?: string };
const isContentEmpty = (content: InputContent) => {
    return match(content)
        .with({ type: "text", text: P.optional("") }, () => true)
        .with({ type: "ai-image", prompt: P.optional("") }, () => true)
        .otherwise(() => false);
};

interface TextMessageInputProps {
    value: InputContent & { type: "text" };
    onChange(content: InputContent): void;
    disabled: boolean;
}

const TextMessageInput: React.FC<TextMessageInputProps> = ({ value, onChange, disabled }) => {
    return (
        <TextField
            fullWidth
            multiline
            value={value.text ?? ""}
            disabled={disabled}
            label="Message"
            onChange={(e) => onChange({ type: "text", text: e.target.value })}
        />
    );
};

interface AiImageMessageInputProps {
    value: InputContent & { type: "ai-image" };
    onChange(content: InputContent): void;
    disabled: boolean;
}

const AiImageMessageInput: React.FC<AiImageMessageInputProps> = ({ value, onChange, disabled }) => {
    return (
        <TextField
            fullWidth
            multiline
            value={value.prompt ?? ""}
            disabled={disabled}
            label="Prompt"
            onChange={(e) => onChange({ type: "ai-image", prompt: e.target.value })}
        />
    );
};

export const SendMessage: React.FC = () => {
    const { currentRoom } = useContext(ChatRoomContext);

    const [content, setContent] = useState<InputContent>({ type: "text", text: "" });

    const handleChangeType = (event: SelectChangeEvent<string>) => {
        match(event.target.value)
            .with("text", () => setContent({ type: "text" }))
            .with("ai-image", () => setContent({ type: "ai-image" }))
            .otherwise(() => setContent({ type: "text" }));
    };

    const [sending, setSending] = useState(false);
    const [error, setError] = useState(false);
    const handleSend = async () => {
        if (currentRoom === undefined || sending) {
            return;
        }

        setSending(true);
        const result = await match(content)
            .with({ type: "text" }, ({ text }) => currentRoom.sendTextMessage(text ?? ""))
            .with({ type: "ai-image" }, ({ prompt }) => currentRoom.sendAiImageMessage(prompt ?? ""))
            .exhaustive()
            .finally(() => setSending(false));

        setError(result.isErr());
        if (result.isOk()) {
            match(content.type)
                .with("text", () => setContent({ type: "text" }))
                .with("ai-image", () => setContent({ type: "ai-image" }))
                .exhaustive();
        }
    };

    const disabled = currentRoom === undefined || sending;

    return (
        <Grid container alignItems="center">
            <Grid item>
                <Select disabled={disabled} value={content.type} onChange={handleChangeType}>
                    <MenuItem value="text">Text</MenuItem>
                    <MenuItem value="ai-image">AI Image</MenuItem>
                </Select>
            </Grid>
            <Grid item xs>
                {content.type === "text" ? (
                    <TextMessageInput disabled={disabled} value={content} onChange={setContent} />
                ) : (
                    <AiImageMessageInput disabled={disabled} value={content} onChange={setContent} />
                )}
            </Grid>
            <Grid item>
                <LoadingButton
                    color={error ? "error" : "primary"}
                    loading={sending}
                    onClick={handleSend}
                    disabled={disabled || isContentEmpty(content)}
                >
                    <Send />
                </LoadingButton>
            </Grid>
        </Grid>
    );
};
