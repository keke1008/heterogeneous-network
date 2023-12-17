import { TextField, TextFieldProps } from "@mui/material";
import { useEffect, useState } from "react";
import * as z from "zod";

type Props<T> = Exclude<TextFieldProps, "value"> & {
    schema: z.Schema<T, z.ZodTypeDef, unknown>;
    stringValue?: string;
    onValue: (value: T | undefined) => void;
    allowEmpty?: boolean;
};

export const ZodSchemaInput = <T,>({ schema, onValue, stringValue, allowEmpty, ...props }: Props<T>) => {
    const [str, setStr] = useState<string>(() => stringValue ?? "");
    useEffect(() => {
        setStr(stringValue ?? "");
    }, [stringValue]);

    const [inValid, setInvalid] = useState<boolean>(false);
    const [touched, setTouched] = useState<boolean>(false);

    useEffect(() => {
        const parsed = str === "" && allowEmpty ? { success: true, data: undefined } : schema.safeParse(str);
        setInvalid(!parsed.success);
        onValue(parsed.success ? parsed.data : undefined);
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [allowEmpty, schema, str]);

    const handleChanged = (e: React.ChangeEvent<HTMLInputElement>) => {
        setTouched(true);
        setStr(e.target.value);
    };

    return <TextField {...props} size="small" error={inValid && touched} value={str} onChange={handleChanged} />;
};
