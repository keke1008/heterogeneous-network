import { ChatApp, ChatRoom } from "@core/apps/chat";
import { createDummy } from "@emulator/ui/contexts/dummy";
import { NetContext } from "@emulator/ui/contexts/netContext";
import React, { useContext, useState } from "react";

export const ChatAppContext = React.createContext<ChatApp>(createDummy("ChatApp"));

export interface ChatConfig {
    aiImageServerAddress?: string;
}

export interface ChatConfigContextType {
    config: ChatConfig;
    setConfig(config: ChatConfig): void;
}

export const ChatConfigContext = React.createContext<ChatConfigContextType>(createDummy("ChatConfigContext"));

export interface ChatRoomContextType {
    rooms: ChatRoom[];
    setRooms(rooms: ChatRoom[]): void;
    currentRoom?: ChatRoom;
    setCurrentRoom(room: ChatRoom): void;
}

export const ChatRoomContext = React.createContext<ChatRoomContextType>(createDummy("ChatRoomContext"));

export const ChatContextProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
    const net = useContext(NetContext);
    const [app] = useState(new ChatApp(net.stream()));

    const [config, setConfig] = useState<ChatConfig>({});
    const [rooms, setRooms] = useState<ChatRoom[]>(app.rooms());
    const [currentRoom, setCurrentRoom] = useState<ChatRoom>();

    return (
        <ChatAppContext.Provider value={app}>
            <ChatConfigContext.Provider value={{ config, setConfig }}>
                <ChatRoomContext.Provider value={{ rooms, setRooms, currentRoom, setCurrentRoom }}>
                    {children}
                </ChatRoomContext.Provider>
            </ChatConfigContext.Provider>
        </ChatAppContext.Provider>
    );
};
