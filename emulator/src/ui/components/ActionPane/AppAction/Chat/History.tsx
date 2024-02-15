import { Message } from "@core/apps/chat";
import { Card, CardContent, Stack } from "@mui/material";
import { ChatMessage } from "./Message";
import { useReducer, useEffect, useContext, useRef } from "react";
import { ChatRoomContext } from "./Context";
import { match } from "ts-pattern";

type MessageAction = { type: "add"; message: Message } | { type: "set"; messages: Message[] };

const messagesReducer = (messages: Message[], action: MessageAction) => {
    return match(action)
        .with({ type: "add" }, ({ message }) => [...messages, message])
        .with({ type: "set" }, ({ messages }) => [...messages])
        .exhaustive()
        .sort((a, b) => a.sentAt.getTime() - b.sentAt.getTime());
};

export const History: React.FC = () => {
    const { currentRoom } = useContext(ChatRoomContext);

    const [messages, dispatchMessages] = useReducer(messagesReducer, []);
    const lastElement = useRef<HTMLDivElement>(null);

    useEffect(() => {
        dispatchMessages({ type: "set", messages: [...(currentRoom?.history ?? [])] });
        return currentRoom?.onMessage((mes) => {
            dispatchMessages({ type: "add", message: mes });
        });
    }, [currentRoom]);

    useEffect(() => {
        lastElement.current?.scrollIntoView({ behavior: "smooth" });
    }, [messages]);

    return (
        <Stack spacing={2} height="100%">
            {messages.map((message, index) => (
                <Card
                    key={message.uuid}
                    ref={index === messages.length - 1 ? lastElement : null}
                    sx={{
                        flexShrink: 0,
                        alignSelf: message.sender === "self" ? "start" : "end",
                    }}
                >
                    <CardContent>
                        <ChatMessage message={message} />
                    </CardContent>
                </Card>
            ))}
        </Stack>
    );
};
