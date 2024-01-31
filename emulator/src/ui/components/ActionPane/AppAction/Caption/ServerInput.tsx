import { ServerInfo } from "@core/apps/caption";
import { Stack, TextField } from "@mui/material";
import { useState, useEffect, useMemo } from "react";
import { z } from "zod";

interface ServerInputProps {
    onChange: (value: ServerInfo) => void;
}

const serverInputSchema = z.object({
    address: z.string(),
    port: z.coerce.number(),
});

export const ServerInput: React.FC<ServerInputProps> = ({ onChange }) => {
    const [input, setInput] = useState({
        address: "",
        port: "",
    });

    const parsed = useMemo(() => serverInputSchema.safeParse(input), [input]);
    useEffect(() => {
        if (parsed.success) {
            onChange(new ServerInfo(parsed.data));
        }
    }, [onChange, parsed]);

    return (
        <Stack spacing={2} direction="row">
            <TextField
                required
                fullWidth
                type="text"
                label="address"
                id="address"
                onChange={(e) => setInput({ ...input, address: e.target.value })}
            />
            <TextField
                required
                fullWidth
                type="number"
                label="port"
                id="port"
                onChange={(e) => setInput({ ...input, port: e.target.value })}
            />
        </Stack>
    );
};
