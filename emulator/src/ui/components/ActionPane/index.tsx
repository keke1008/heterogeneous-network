import React from "react";
import { Divider, Grid, Tab, Tabs } from "@mui/material";
import { Link, Outlet } from "react-router-dom";

const routes = [
    { name: "network", path: "/" },
    { name: "apps", path: "/apps" },
] as const;

export const ActionPane: React.FC = () => {
    const [selectedTab, setSelectedTab] = React.useState<string>("network");

    return (
        <Grid container direction="column" height="100%">
            <Grid item>
                <Tabs variant="fullWidth" value={selectedTab} onChange={(_, value) => setSelectedTab(value)}>
                    {routes.map((route) => (
                        <Tab key={route.name} value={route.name} label={route.name} component={Link} to={route.path} />
                    ))}
                </Tabs>
            </Grid>
            <Grid item>
                <Divider />
            </Grid>
            <Grid item xs>
                <Outlet />
            </Grid>
        </Grid>
    );
};
