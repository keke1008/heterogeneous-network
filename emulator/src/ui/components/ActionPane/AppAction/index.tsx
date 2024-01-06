import { Divider, Grid, List, ListItemButton } from "@mui/material";
import { Link, Outlet } from "react-router-dom";

export const AppAction = () => (
    <Grid container spacing={2}>
        <Grid item xs={3}>
            <List>
                <ListItemButton component={Link} to={"echo"}>
                    Echo
                </ListItemButton>
            </List>
        </Grid>

        <Grid item xs={0}>
            <Divider orientation="vertical" />
        </Grid>

        <Grid item margin={2}>
            <Outlet />
        </Grid>
    </Grid>
);
