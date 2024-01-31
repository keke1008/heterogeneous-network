import { Divider, Grid, List, ListItemButton } from "@mui/material";
import { Link, Outlet } from "react-router-dom";

export const AppAction = () => (
    <Grid container height="100%">
        <Grid item xs height="100%">
            <List>
                <ListItemButton component={Link} to={"echo"}>
                    Echo
                </ListItemButton>
                <ListItemButton component={Link} to={"caption"}>
                    Caption
                </ListItemButton>
                <ListItemButton component={Link} to={"file"}>
                    File
                </ListItemButton>
            </List>
        </Grid>

        <Grid item xs={0} height="100%">
            <Divider orientation="vertical" />
        </Grid>

        <Grid item xs={8} height="100%" flexGrow={1} padding={2}>
            <Outlet />
        </Grid>
    </Grid>
);
