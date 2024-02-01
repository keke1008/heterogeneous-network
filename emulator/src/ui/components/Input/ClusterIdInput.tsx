import { ClusterId } from "@core/net";
import { TextField } from "@mui/material";
import { useState, useEffect } from "react";
import { ZodValidatedInput } from ".";

interface ClusterIdInputProps {
    value?: ClusterId;
    defaultValue?: ClusterId;
    onChange: (clusterId: ClusterId) => void;
    inputProps?: React.ComponentProps<typeof TextField>;
}

export const ClusterIdInput: React.FC<ClusterIdInputProps> = ({ value, defaultValue, onChange, inputProps }) => {
    const [body, setBody] = useState(defaultValue?.toHumanReadableString() ?? "0");
    const [error, setError] = useState(false);

    useEffect(() => {
        if (value !== undefined) {
            setBody(value.toHumanReadableString());
        }
    }, [value]);

    useEffect(() => {
        const result = ClusterId.noClusterExcludedSchema.safeParse(body);
        setError(!result.success);
        if (result.success) {
            onChange(result.data);
        }
    }, [body, onChange]);

    return <TextField {...inputProps} fullWidth error={error} value={body} onChange={(e) => setBody(e.target.value)} />;
};

interface ControlledClusterIdInputProps {
    value: string;
    onChange(value: string): void;
    inputProps?: React.ComponentProps<typeof TextField>;
}
export const ControlledClusterIdInput: React.FC<ControlledClusterIdInputProps> = ({ value, onChange, inputProps }) => {
    return (
        <ZodValidatedInput
            {...inputProps}
            schema={ClusterId.noClusterExcludedSchema}
            value={value}
            onChange={(e) => onChange(e.target.value)}
        />
    );
};
