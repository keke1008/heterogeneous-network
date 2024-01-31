import { Divider, Grid, List, ListItemButton } from "@mui/material";
import { useState } from "react";
import { Link, Outlet } from "react-router-dom";
import { actions } from "./actions";

export const AppAction = () => {
    const [selected, setSelected] = useState<string>();

    return (
        <Grid container height="100%">
            <Grid item xs height="100%">
                <List sx={{ height: "100%", overflow: "scroll" }}>
                    {actions.map(({ name, path }) => (
                        <ListItemButton
                            key={name}
                            component={Link}
                            to={path}
                            selected={selected === name}
                            onClick={() => setSelected(name)}
                        >
                            {name}
                        </ListItemButton>
                    ))}
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
};
