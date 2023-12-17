import { Address, NodeId } from "@core/net";
import { AddressInput } from "./AddressInput";
import { TextField } from "@mui/material";

export interface Props {
    onValue: (nodeId: NodeId | undefined) => void;
    label: string;
    stringValue?: string;
    allowEmpty?: boolean;
    textProps?: React.ComponentProps<typeof TextField>;
}

export const NodeIdInput: React.FC<Props> = ({ onValue, label, stringValue, allowEmpty, textProps }) => {
    const handleValue = (address: Address | undefined) => onValue(address && new NodeId(address));
    return (
        <AddressInput
            onValue={handleValue}
            label={label}
            stringValue={stringValue}
            allowEmpty={allowEmpty}
            textProps={textProps}
        />
    );
};
