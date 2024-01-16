import React, { useEffect, useState } from "react";
import { ActionButton } from "../ActionTemplates";
import { Stack } from "@mui/material";

export interface Client {
    close: () => Promise<void>;
}

export type ConnectResult<C extends Client> = { type: "success"; client: C } | { type: "failure"; reason: string };

export interface Props<C extends Client> {
    children: (client: C) => React.ReactNode;
    connect: () => Promise<ConnectResult<C>>;
}

export const ConnectionButton = <C extends Client>({ children, connect }: Props<C>) => {
    const [client, setClient] = useState<C>();

    useEffect(() => {
        return () => {
            client?.close();
        };
    }, [client]);

    if (client === undefined) {
        const handleConnect = async () => {
            const result = await connect();
            result.type === "success" && setClient(result.client);
            return result;
        };
        return <ActionButton onClick={handleConnect}>Connect</ActionButton>;
    } else {
        const handleClose = async () => {
            await client.close();
            setClient(undefined);
            return { type: "success" } as const;
        };
        return (
            <Stack>
                <ActionButton onClick={handleClose}>Disconnect</ActionButton>
                {children(client)}
            </Stack>
        );
    }
};
