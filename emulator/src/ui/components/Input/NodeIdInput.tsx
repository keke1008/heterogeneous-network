import { Address, NodeId } from "@core/net";
import { AddressInput } from "./AddressInput";

export interface Props {
    onChange: (nodeId: NodeId | undefined) => void;
    label: string;
}

export const NodeIdInput: React.FC<Props> = ({ onChange, label }) => {
    const handleChange = (address: Address | undefined) => onChange(address && new NodeId(address));
    return AddressInput({ onChange: handleChange, label });
};
