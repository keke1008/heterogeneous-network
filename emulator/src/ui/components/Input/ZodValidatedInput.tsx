import { TextField, TextFieldProps } from "@mui/material";
import { useMemo, useState } from "react";
import { z } from "zod";

type Props = Omit<TextFieldProps, "error"> & {
    schema: z.Schema<unknown, z.ZodTypeDef, unknown>;
};

export const ZodValidatedInput = ({ schema, value, children, ...props }: Props) => {
    const [touched, setTouched] = useState(false);
    const touch = () => setTouched(true);

    const errorMessage = useMemo(() => {
        const result = schema.safeParse(value);
        return result.success ? undefined : result.error.format()._errors.join(", ");
    }, [schema, value]);

    const error = touched && errorMessage !== undefined;

    const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        touch();
        props.onChange?.(e);
    };

    return (
        <TextField
            {...props}
            value={value}
            error={error}
            helperText={error ? errorMessage : props.helperText}
            onChange={handleChange}
            onBlur={touch}
        >
            {children}
        </TextField>
    );
};
