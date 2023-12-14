import React from "react";
import { Divider, Stack, Typography } from "@mui/material";
import { NodeId } from "@core/net";
import { Debug } from "./Debug";
import { Media } from "./Media";
import { Wifi } from "./Wifi";
import { Neighbor } from "./Neighbor";
import { VRouter } from "./VRouter";

export const Actions: React.FC<{ targetId: NodeId }> = ({ targetId }) => {
    return (
        <Stack spacing={1} paddingY={1} divider={<Divider />}>
            <Typography variant="h4" sx={{ textAlign: "center" }}>
                {targetId.toString()}
            </Typography>

            <Debug targetId={targetId} />
            <Media targetId={targetId} />
            <Wifi targetId={targetId} />
            <Neighbor targetId={targetId} />
            <VRouter targetId={targetId} />
        </Stack>
    );
};
