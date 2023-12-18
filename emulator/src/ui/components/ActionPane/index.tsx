import React, { useEffect } from "react";
import { Box, Divider, Stack, Tab, Tabs, Typography } from "@mui/material";
import { Source } from "@core/net";
import { Debug } from "./Debug";
import { Media } from "./Media";
import { Wifi } from "./Wifi";
import { Neighbor } from "./Neighbor";
import { VRouter } from "./VRouter";
import { Local } from "./Local";
import { Serial } from "./Serial";

const tabNames = ["Local", "Selection"] as const;
type TabName = (typeof tabNames)[number];

interface Props {
    selectedNode?: Source;
    localNode?: Source;
}

export const ActionPane: React.FC<Props> = ({ selectedNode, localNode }) => {
    const [selectedTab, setSelectedTab] = React.useState<TabName>("Local");

    const displayNode = selectedTab === "Local" ? localNode : selectedNode;
    const targetNode = displayNode?.intoDestination();

    useEffect(() => {
        if (selectedNode === undefined) {
            setSelectedTab("Local");
        } else if (selectedNode && localNode?.equals(selectedNode)) {
            setSelectedTab("Local");
        } else {
            setSelectedTab("Selection");
        }
    }, [selectedNode, localNode]);

    return (
        <Box>
            <Tabs variant="fullWidth" value={selectedTab} onChange={(_, value) => setSelectedTab(value)}>
                {tabNames.map((tabName) => (
                    <Tab key={tabName} label={tabName} value={tabName} />
                ))}
            </Tabs>
            <Divider />
            {targetNode && (
                <>
                    <Typography variant="h4" sx={{ textAlign: "center" }}>
                        {displayNode?.display()}
                    </Typography>

                    <Stack spacing={1} paddingY={1} divider={<Divider />}>
                        {selectedTab === "Local" && <Local />}
                        <Debug targetNode={targetNode} />
                        <Media targetNode={targetNode} />
                        <Wifi targetNode={targetNode} />
                        <Serial targetNode={targetNode} />
                        <Neighbor targetNode={targetNode} />
                        <VRouter targetNode={targetNode} />
                    </Stack>
                </>
            )}
        </Box>
    );
};
