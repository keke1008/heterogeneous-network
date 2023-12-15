import { TextField, TextFieldProps } from "@mui/material";
import { useEffect, useMemo, useState } from "react";
import * as z from "zod";

type Props<T> = TextFieldProps & {
    schema: z.ZodType<T, z.ZodTypeDef, unknown>;
    onValue?: (value: T | undefined) => void;
    allowEmpty?: T;
};

export const ZodSchemaInput = <T,>({ schema, onValue, ...props }: Props<T>) => {
    const [str, setStr] = useState<string>("");

    const { allowEmpty, ...textFieldProps } = props;
    const parsed = useMemo(() => {
        if (str === "" && "allowEmpty" in props) {
            return { success: true, data: allowEmpty };
        } else {
            return schema.safeParse(str);
        }
    }, [allowEmpty, props, schema, str]);

    useEffect(() => {
        onValue?.(parsed.success ? parsed.data : undefined);
    }, [onValue, parsed]);

    const [touched, setTouched] = useState<boolean>(false);
    const handleChanged = (e: React.ChangeEvent<HTMLInputElement>) => {
        setStr(e.target.value);
        setTouched(true);
    };
    const error = touched && !parsed.success;

    return <TextField {...textFieldProps} size="small" error={error} onChange={handleChanged} />;
};
