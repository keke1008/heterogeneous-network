import React, { useEffect } from "react";
import { Box, Divider, Stack, Tab, Tabs, Typography } from "@mui/material";
import { NodeId } from "@core/net";
import { Debug } from "./Debug";
import { Media } from "./Media";
import { Wifi } from "./Wifi";
import { Neighbor } from "./Neighbor";
import { VRouter } from "./VRouter";
import { Local } from "./Local";

const Actions: React.FC<{ targetId: NodeId }> = ({ targetId }) => {
    return (
        <>
            <Debug targetId={targetId} />
            <Media targetId={targetId} />
            <Wifi targetId={targetId} />
            <Neighbor targetId={targetId} />
            <VRouter targetId={targetId} />
        </>
    );
};

const tabNames = ["Local", "Selection"] as const;
type TabName = (typeof tabNames)[number];

interface Props {
    selectedNodeId?: NodeId;
    localNodeId?: NodeId;
}

export const ActionPane: React.FC<Props> = ({ selectedNodeId, localNodeId }) => {
    const [selectedTab, setSelectedTab] = React.useState<TabName>("Local");

    const targetId = selectedTab === "Local" ? localNodeId : selectedNodeId;

    useEffect(() => {
        setSelectedTab(selectedNodeId ? "Selection" : "Local");
    }, [selectedNodeId]);

    return (
        <Box>
            <Tabs variant="fullWidth" value={selectedTab} onChange={(_, value) => setSelectedTab(value)}>
                {tabNames.map((tabName) => (
                    <Tab key={tabName} label={tabName} value={tabName} />
                ))}
            </Tabs>
            <Divider />
            {targetId && (
                <>
                    <Typography variant="h4" sx={{ textAlign: "center" }}>
                        {targetId.toString()}
                    </Typography>

                    <Stack spacing={1} paddingY={1} divider={<Divider />}>
                        {selectedTab === "Local" && <Local />}
                        <Actions targetId={targetId} />
                    </Stack>
                </>
            )}
        </Box>
    );
};
