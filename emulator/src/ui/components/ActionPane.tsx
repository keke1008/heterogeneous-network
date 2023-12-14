import React, { useEffect } from "react";
import { Tabs, Tab, Box, Divider } from "@mui/material";
import { NodeId } from "@core/net";
import { Actions } from "./Actions";

const tabNames = ["Local", "Selection"] as const;
type TabName = (typeof tabNames)[number];

interface Props {
    selectedNodeId: NodeId | undefined;
    localNodeId: NodeId;
}

export const ActionPane: React.FC<Props> = ({ selectedNodeId, localNodeId }) => {
    const [selectedTab, setSelectedTab] = React.useState<TabName>("Local");

    const targetNodeId = selectedTab === "Local" ? localNodeId : selectedNodeId;

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
            {targetNodeId && <Actions targetId={targetNodeId} />}
        </Box>
    );
};
