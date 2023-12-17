import { ClusterId, Destination, NodeId } from "@core/net";
import { Stack } from "@mui/material";
import { useEffect, useState } from "react";
import { NodeIdInput } from "./NodeIdInput";
import { ZodSchemaInput } from "./ZodSchemaInput";

interface Props {
    onValue: (destination: Destination) => void;
    nodeIdProps?: React.ComponentProps<typeof NodeIdInput>;
    clusterIdProps?: React.ComponentProps<typeof ZodSchemaInput>;
}

export const DestinationInput: React.FC<Props> = ({ onValue, nodeIdProps, clusterIdProps }) => {
    const [nodeId, setNodeId] = useState<NodeId>();
    const [clusterId, setClusterId] = useState<ClusterId>();

    useEffect(() => {
        const destination = new Destination({ nodeId, clusterId });
        onValue(destination);
    }, [nodeId, clusterId, onValue]);

    return (
        <Stack direction="row" spacing={2}>
            <NodeIdInput {...nodeIdProps} label="node id" onValue={setNodeId} allowEmpty={true} />
            <ZodSchemaInput<ClusterId | undefined>
                {...clusterIdProps}
                label="cluster id"
                onValue={setClusterId}
                schema={ClusterId.noClusterExcludedSchema}
                allowEmpty={true}
            />
        </Stack>
    );
};
