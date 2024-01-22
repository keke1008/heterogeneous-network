import { Address, AddressType, SerialAddress, UdpAddress, UhfAddress, WebSocketAddress } from "@core/net";
import { Stack, TextField } from "@mui/material";
import React, { useMemo, useState } from "react";
import { AddressTypeSelector } from "./AddressTypeSelector";
import { ZodSchemaInput } from "../ZodSchemaInput";
import * as z from "zod";

const getAddressClass = (type: AddressType) => {
    switch (type) {
        case AddressType.Serial:
            return SerialAddress;
        case AddressType.Uhf:
            return UhfAddress;
        case AddressType.Udp:
            return UdpAddress;
        case AddressType.WebSocket:
            return WebSocketAddress;
        default:
            throw new Error("Invalid address type");
    }
};

export interface Props {
    onValue: (address: Address | undefined) => void;
    label: string;
    types?: AddressType[];
    stringValue?: string;
    allowEmpty?: boolean;
    textProps?: React.ComponentProps<typeof TextField>;
}

export const AddressInput: React.FC<Props> = ({ onValue, label, types, stringValue, textProps, allowEmpty }) => {
    const [selection, setSelection] = useState<AddressType>(types ? types[0] : AddressType.Serial);
    const schema: z.Schema<Address, z.ZodTypeDef, unknown> = useMemo(() => {
        return getAddressClass(selection).schema.transform((v) => new Address(v));
    }, [selection]);

    return (
        <Stack direction="row">
            <AddressTypeSelector addressType={selection} onChange={setSelection} types={types} />
            <ZodSchemaInput<Address | undefined>
                {...textProps}
                label={label}
                schema={schema}
                stringValue={stringValue}
                onValue={onValue}
                allowEmpty={allowEmpty}
                InputProps={{ sx: { borderStartStartRadius: 0, borderEndStartRadius: 0 } }}
            />
        </Stack>
    );
};
