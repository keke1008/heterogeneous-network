import { Divider, Grid, List, ListItem, ListItemButton, ListSubheader } from "@mui/material";
import { Link, Outlet } from "react-router-dom";
import { actionGroups } from "./actions";
import { useState } from "react";
import React from "react";

export const NetworkAction: React.FC = () => {
    const [selected, setSelected] = useState<string>();

    return (
        <Grid container height="100%" alignItems="start">
            <Grid item xs height="100%">
                <List sx={{ height: "100%", overflow: "scroll" }}>
                    {actionGroups.map(({ groupName, actions }) => (
                        <React.Fragment key={groupName}>
                            <ListSubheader disableSticky>{groupName}</ListSubheader>
                            {actions.map(({ name, path }) => (
                                <ListItem key={name} sx={{ paddingY: 0 }}>
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
                        </React.Fragment>
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
