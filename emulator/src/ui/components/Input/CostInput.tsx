import { Cost } from "@core/net";
import { TextField } from "@mui/material";

interface Props {
    onChange: (cost: Cost | undefined) => void;
    label: string;
}

export const CostInput: React.FC<Props> = ({ onChange, label }) => {
    const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        const cost = Cost.fromHumanReadableString(e.target.value);
        onChange(cost.isOk() ? cost.unwrap() : undefined);
    };

    return <TextField size="small" type="number" label={label} onChange={handleChange} />;
};
