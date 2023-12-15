import { Grid } from "@mui/material";

interface Props {
    children: React.ReactNode;
}

export const Action: React.FC<Props> = ({ children }) => {
    return (
        <Grid container gap={2}>
            {children}
        </Grid>
    );
};
