import { ChatApp, ChatRoom } from "@core/apps/chat";
import { AppContext } from "@emulator/ui/contexts/appContext";
import { createDummy } from "@emulator/ui/contexts/dummy";
import React, { useContext, useEffect, useState } from "react";

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
    currentRoom?: ChatRoom;
    setCurrentRoom(room: ChatRoom | undefined): void;
}

export const ChatRoomContext = React.createContext<ChatRoomContextType>(createDummy("ChatRoomContext"));

export const ChatContextProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
    const app = useContext(AppContext).chatApp();

    const [config, setConfig] = useState<ChatConfig>({});
    const [rooms, setRooms] = useState<ChatRoom[]>(app.rooms());
    const [currentRoom, setCurrentRoom] = useState<ChatRoom>();

    useEffect(() => {
        const cancel1 = app.onRoomCreated((room) => {
            setRooms((rooms) => [...rooms, room]);
        });
        const cancel2 = app.onRoomClosed((room) => {
            setRooms((rooms) => rooms.filter((r) => r.uuid !== room.uuid));
            if (currentRoom?.uuid === room.uuid) {
                setCurrentRoom(undefined);
            }
        });

        return () => {
            cancel1();
            cancel2();
        };
    }, [app, currentRoom]);

    return (
        <ChatAppContext.Provider value={app}>
            <ChatConfigContext.Provider value={{ config, setConfig }}>
                <ChatRoomContext.Provider value={{ rooms, currentRoom, setCurrentRoom }}>
                    {children}
                </ChatRoomContext.Provider>
            </ChatConfigContext.Provider>
        </ChatAppContext.Provider>
    );
};
