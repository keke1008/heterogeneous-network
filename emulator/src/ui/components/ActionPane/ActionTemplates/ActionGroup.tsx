import { Grid, Typography } from "@mui/material";

interface Props {
    name: string;
    children: React.ReactNode;
}

export const ActionGroup: React.FC<Props> = ({ name, children }) => {
    return (
        <>
            <Typography variant="subtitle2">{name}</Typography>
            <Grid container gap={2}>
                {children}
            </Grid>
        </>
    );
};
