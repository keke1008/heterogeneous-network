import { ChatRoom, Message } from "@core/apps/chat";
import { Stack } from "@mui/material";
import { ChatMessage } from "./Message";
import { useReducer, useEffect } from "react";

interface HistoryProps {
    room: ChatRoom;
    onClose(): void;
}

export const History: React.FC<HistoryProps> = ({ room, onClose }) => {
    const [messages, addMessages] = useReducer(
        (messages: Message[], message: Message) => {
            messages.push(message);
            return messages.sort();
        },
        room,
        (room) => [...room.history].sort(),
    );

    useEffect(() => {
        room.onMessage(addMessages);
    }, [room]);

    useEffect(() => {
        room.onClose(onClose);
    }, [room, onClose]);

    return (
        <Stack spacing={2}>
            {messages.map((message) => (
                <ChatMessage key={message.sentAt.toString()} message={message} />
            ))}
        </Stack>
    );
};
