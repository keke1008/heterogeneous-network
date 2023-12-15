import { Grid } from "@mui/material";

interface Props {
    children: React.ReactNode;
}

export const ActionParameter: React.FC<Props> = ({ children }) => {
    return <Grid item>{children}</Grid>;
};
