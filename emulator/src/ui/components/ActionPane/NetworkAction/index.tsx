import { Box, Divider, Grid, List, ListItem, ListItemButton, ListSubheader } from "@mui/material";
import { Link, Outlet } from "react-router-dom";
import { actionGroups } from "./actions";
import { useState } from "react";

export const NetworkAction: React.FC = () => {
    const [selected, setSelected] = useState<string>();

    return (
        <Grid container spacing={2}>
            <Grid item xs>
                <List sx={{ height: "100%", overflow: "scroll" }}>
                    {actionGroups.map(({ groupName, actions }) => (
                        <Box key={groupName}>
                            <ListSubheader disableSticky>{groupName}</ListSubheader>
                            {actions.map(({ name, path }) => (
                                <ListItem key={name}>
                                    <ListItemButton
                                        component={Link}
                                        to={path}
                                        selected={name === selected}
                                        onClick={() => setSelected(name)}
                                    >
                                        {name}
                                    </ListItemButton>
                                </ListItem>
                            ))}
                        </Box>
                    ))}
                </List>
            </Grid>
            <Grid item xs={0}>
                <Divider orientation="vertical" />
            </Grid>
            <Grid item xs={8} flexGrow={1} margin={2}>
                <Outlet />
            </Grid>
        </Grid>
    );
};
