import { AddressType } from "@core/net";
import { ToggleButtonGroup, ToggleButton } from "@mui/material";
import { useEffect, useReducer } from "react";

interface Props {
    addressType: AddressType | undefined;
    onChange: (addressType: AddressType) => void;
    types?: AddressType[];
}

const DEFAULT_ADDRESS_TYPE_LIST = [AddressType.Serial, AddressType.Uhf, AddressType.Udp, AddressType.WebSocket];

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

export const AddressTypeSelector: React.FC<Props> = ({ addressType, onChange, types }) => {
    types ??= DEFAULT_ADDRESS_TYPE_LIST;
    const [selection, dispatchSelection] = useReducer(
        (prev: AddressType, next: AddressType | undefined) => next ?? prev,
        addressType ?? types[0],
    );

    const handleChange = (_: unknown, type: AddressType | undefined) => {
        type && dispatchSelection(type);
    };

    useEffect(() => {
        onChange(selection);
    }, [selection, onChange]);

    return (
        <ToggleButtonGroup size="small" exclusive value={selection} onChange={handleChange}>
            {types.map((type) => (
                <ToggleButton value={type} key={type} sx={{ borderTopRightRadius: 0, borderBottomRightRadius: 0 }}>
                    {typeToName(type)}
                </ToggleButton>
            ))}
        </ToggleButtonGroup>
    );
};
