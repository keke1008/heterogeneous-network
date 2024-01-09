import { Divider, Grid, List, ListItemButton } from "@mui/material";
import { Link, Outlet } from "react-router-dom";

export const AppAction = () => (
    <Grid container spacing={2}>
        <Grid item xs={3}>
            <List>
                <ListItemButton component={Link} to={"echo"}>
                    Echo
                </ListItemButton>
                <ListItemButton component={Link} to={"caption"}>
                    Caption
                </ListItemButton>
            </List>
        </Grid>

        <Grid item xs={0}>
            <Divider orientation="vertical" />
        </Grid>

        <Grid item flexGrow={1} margin={2}>
            <Outlet />
        </Grid>
    </Grid>
);
