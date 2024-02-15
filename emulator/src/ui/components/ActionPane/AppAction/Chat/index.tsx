import { ChatRoom, Message } from "@core/apps/chat";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { AppContext } from "@emulator/ui/contexts/appContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { Stack } from "@mui/material";
import { useContext, useEffect, useReducer } from "react";
import { ChatMessage } from "./Message";

export const ChatApp: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);
    const app = useContext(AppContext);
};
