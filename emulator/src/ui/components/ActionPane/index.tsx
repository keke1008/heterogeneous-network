import React from "react";
import { Box, Divider, Stack, Tab, Tabs } from "@mui/material";
import { Link, Outlet } from "react-router-dom";
import { usePosting } from "./AppAction/usePosting";

const routes = [
    { name: "network", path: "/network" },
    { name: "apps", path: "/apps" },
] as const;

export const ActionPane: React.FC = () => {
    const [selectedTab, setSelectedTab] = React.useState<string>("network");
    usePosting();

    return (
        <Stack height="100%">
            <Tabs variant="fullWidth" value={selectedTab} onChange={(_, value) => setSelectedTab(value)}>
                {routes.map((route) => (
                    <Tab key={route.name} value={route.name} label={route.name} component={Link} to={route.path} />
                ))}
            </Tabs>
            <Divider />
            <Box sx={{ flexGrow: 1, overflowY: "scroll" }}>
                <Outlet />
            </Box>
        </Stack>
    );
};
