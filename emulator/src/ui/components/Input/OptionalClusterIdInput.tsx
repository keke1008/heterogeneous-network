import { ClusterId } from "@core/net";
import { OptionalClusterId } from "@core/net/node/clusterId";
import { TextField } from "@mui/material";
import { useState, useEffect } from "react";

interface OptionalClusterIdInputProps {
    value: OptionalClusterId | undefined;
    onChange: (clusterId: OptionalClusterId) => void;
}

export const OptionalClusterIdInput: React.FC<OptionalClusterIdInputProps> = ({ value, onChange }) => {
    const [body, setBody] = useState("0");
    const [error, setError] = useState(false);

    useEffect(() => {
        if (value !== undefined) {
            setBody(value.toHumanReadableString());
        }
    }, [value]);

    useEffect(() => {
        const result = ClusterId.schema.safeParse(body);
        setError(!result.success);
        if (result.success) {
            onChange(result.data);
        }
    }, [body, onChange]);

    return (
        <TextField fullWidth error={error} label="Cluster ID" value={body} onChange={(e) => setBody(e.target.value)} />
    );
};
