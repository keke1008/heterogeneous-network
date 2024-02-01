import { ClusterId, Destination, NodeId } from "@core/net";
import { OptionalClusterId } from "@core/net/node/clusterId";
import { Grid, Typography } from "@mui/material";
import { useEffect, useState } from "react";
import { NodeIdInput } from "./NodeIdInput";
import { OptionalClusterIdInput } from "./OptionalClusterIdInput";

interface Props {
    value: Destination | undefined;
    onChange: (destination: Destination | undefined) => void;
}

export const DestinationInput: React.FC<Props> = ({ value, onChange }) => {
    const [nodeId, setNodeId] = useState<NodeId>();
    const [clusterId, setClusterId] = useState<OptionalClusterId>();

    useEffect(() => {
        setNodeId(value?.nodeId);
        setClusterId(value?.clusterId);
    }, [value]);

    useEffect(() => {
        const destination = new Destination({ nodeId, clusterId: clusterId ?? ClusterId.noCluster() });
        onChange(destination);
    }, [clusterId, nodeId, onChange]);

    return (
        <Grid container spacing={2} marginTop={1}>
            <Grid item xs={6}>
                <Typography variant="body1" sx={{ textAlign: "center" }}>
                    Node ID: {nodeId?.toHumanReadableString()}
                </Typography>
            </Grid>
            <Grid item xs={6}>
                <Typography variant="body1" sx={{ textAlign: "center" }}>
                    Cluster ID: {clusterId?.display()}
                </Typography>
            </Grid>
            <Grid item xs={12}>
                <NodeIdInput value={nodeId} onChange={setNodeId} />
            </Grid>
            <Grid item xs={12}>
                <OptionalClusterIdInput value={clusterId} onChange={setClusterId} />
            </Grid>
        </Grid>
    );
};
