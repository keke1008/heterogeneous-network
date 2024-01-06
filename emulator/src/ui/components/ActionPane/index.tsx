import React, { useContext } from "react";
import { Box, Divider, Stack, Tab, Tabs, Typography } from "@mui/material";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { Local, Debug, Media, Wifi, Serial, Neighbor, VRouter } from "./Network";

const tabNames = ["Local", "Selection"] as const;
type TabName = (typeof tabNames)[number];

export const ActionPane: React.FC = () => {
    const [selectedTab, setSelectedTab] = React.useState<TabName>("Local");
    const { target } = useContext(ActionContext);

    return (
        <Box>
            <Tabs variant="fullWidth" value={selectedTab} onChange={(_, value) => setSelectedTab(value)}>
                {tabNames.map((tabName) => (
                    <Tab key={tabName} label={tabName} value={tabName} />
                ))}
            </Tabs>
            <Divider />

            {target && (
                <>
                    <Typography variant="h4" sx={{ textAlign: "center" }}>
                        {target?.nodeId.toString()}, {target?.clusterId.toString()}
                    </Typography>

                    <Stack spacing={1} paddingY={1} divider={<Divider />}>
                        <Local />
                        <Debug />
                        <Media />
                        <Wifi />
                        <Serial />
                        <Neighbor />
                        <VRouter />
                    </Stack>
                </>
            )}
        </Box>
    );
};
