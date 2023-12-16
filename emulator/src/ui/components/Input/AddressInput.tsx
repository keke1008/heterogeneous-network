import { Address, AddressType, SerialAddress, UdpAddress, UhfAddress, WebSocketAddress } from "@core/net";
import { DeserializeResult } from "@core/serde";
import { TextField, ToggleButton, ToggleButtonGroup } from "@mui/material";
import React, { useEffect, useMemo, useReducer, useState } from "react";

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

const typeToName = (type: AddressType): string => {
    switch (type) {
        case AddressType.Serial:
            return "Serial";
        case AddressType.Uhf:
            return "UHF";
        case AddressType.Udp:
            return "UDP";
        case AddressType.WebSocket:
            return "WS";
        default:
            throw new Error("Invalid address type");
    }
};

const typeToLabel = (type: AddressType): string => {
    return `${typeToName(type)} address`;
};

type AddressDeserializeResult = DeserializeResult<SerialAddress | UhfAddress | UdpAddress | WebSocketAddress>;

export interface Props {
    onChange: (address: Address | undefined) => void;
    label: string;
    types?: AddressType[];
    initialType?: AddressType;
    stringValue?: string;
    autoFocus?: boolean;
}

const ADDRESS_TYPES = [AddressType.Serial, AddressType.Uhf, AddressType.Udp, AddressType.WebSocket];

export const AddressInput: React.FC<Props> = ({ onChange, label, types, initialType, stringValue, ...props }) => {
    types ??= ADDRESS_TYPES;
    initialType ??= types[0];

    const [type, dispatchType] = useReducer(
        (prev: AddressType, next: AddressType | undefined) => next ?? prev ?? initialType,
        initialType,
    );
    const [addressBody, setAddressBody] = useState<string>(() => stringValue ?? "");
    const [touched, setTouched] = useState<boolean>(false);

    useEffect(() => {
        stringValue && setAddressBody(stringValue);
    }, [stringValue]);

    const address = useMemo(() => {
        const addressClass = getAddressClass(type);
        const result: AddressDeserializeResult = addressClass.fromHumanReadableString(addressBody);
        return result.isOk() ? new Address(result.unwrap()) : undefined;
    }, [type, addressBody]);

    useEffect(() => {
        onChange(address);
    }, [address, onChange]);

    const handleTextChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        setAddressBody(e.target.value);
        setTouched(true);
    };

    return (
        <div>
            <ToggleButtonGroup size="small" exclusive value={type} onChange={(_, type) => dispatchType(type)}>
                {types.map((type) => (
                    <ToggleButton value={type} key={type} sx={{ borderTopRightRadius: 0, borderBottomRightRadius: 0 }}>
                        {typeToName(type)}
                    </ToggleButton>
                ))}
            </ToggleButtonGroup>
            <TextField
                {...props}
                InputProps={{ sx: { borderTopLeftRadius: 0, borderBottomLeftRadius: 0 } }}
                size="small"
                placeholder={typeToLabel(type)}
                label={label}
                error={touched && address === undefined}
                value={addressBody}
                onChange={handleTextChange}
            />
        </div>
    );
};
