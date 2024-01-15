import { ClusterId, Destination, NodeId, SerialAddress, UdpAddress, UhfAddress, WebSocketAddress } from "@core/net";
import { OptionalClusterId } from "@core/net/node/clusterId";
import { NodeIdType } from "@core/net/node/nodeId";
import { Grid, Stack, TextField, ToggleButton, ToggleButtonGroup, Typography } from "@mui/material";
import { useEffect, useState } from "react";
import { match } from "ts-pattern";

interface NodeIdInputProps {
    value: NodeId | undefined;
    onChange: (nodeId: NodeId) => void;
}

const nodeIdTypes = [
    NodeIdType.Broadcast,
    NodeIdType.Loopback,
    NodeIdType.Serial,
    NodeIdType.UHF,
    NodeIdType.IPv4,
    NodeIdType.WebSocket,
];

const nodeIdTypeToLabel = (type: NodeIdType) => {
    return match(type)
        .with(NodeIdType.Broadcast, () => "Broadcast")
        .with(NodeIdType.Loopback, () => "Loopback")
        .with(NodeIdType.Serial, () => "Serial")
        .with(NodeIdType.UHF, () => "UHF")
        .with(NodeIdType.IPv4, () => "UDP")
        .with(NodeIdType.WebSocket, () => "WS")
        .run();
};

const intoBodySchema = (type: NodeIdType) => {
    return match(type)
        .with(NodeIdType.Serial, () => SerialAddress.schema)
        .with(NodeIdType.UHF, () => UhfAddress.schema)
        .with(NodeIdType.IPv4, () => UdpAddress.schema)
        .with(NodeIdType.WebSocket, () => WebSocketAddress.schema)
        .otherwise(() => undefined);
};

const createNodeIdFromTypeAndBodyString = (type: NodeIdType, body: string): NodeId | undefined => {
    if (type === NodeIdType.Broadcast) {
        return NodeId.broadcast();
    } else if (type === NodeIdType.Loopback) {
        return NodeId.loopback();
    }

    const schema = intoBodySchema(type);
    if (schema === undefined) {
        throw new Error(`Invalid node ID type ${type}`);
    }

    const result = schema.safeParse(body);
    if (!result.success) {
        return undefined;
    }

    return NodeId.fromAddress(result.data);
};

const isBodylessType = (type: NodeIdType) => {
    return match(type)
        .with(NodeIdType.Broadcast, () => true)
        .with(NodeIdType.Loopback, () => true)
        .otherwise(() => false);
};

const NodeIdInput: React.FC<NodeIdInputProps> = ({ value, onChange }) => {
    const [type, setType] = useState<NodeIdType>(NodeIdType.Broadcast);
    const [body, setBody] = useState("");
    const [error, setError] = useState(false);

    useEffect(() => {
        if (value !== undefined) {
            setType(value.type());
            const body = value.toHumanReadableBodyString();
            if (body !== "") {
                setBody(body);
            }
        }
    }, [value]);

    useEffect(() => {
        const nodeId = createNodeIdFromTypeAndBodyString(type, body);
        setError(nodeId === undefined);
        if (nodeId !== undefined) {
            onChange(nodeId);
        }
    }, [type, body, onChange]);

    return (
        <Stack direction="row">
            <ToggleButtonGroup size="small" value={type} exclusive onChange={(_, v) => setType((prev) => v ?? prev)}>
                {nodeIdTypes.map((type) => (
                    <ToggleButton key={type} value={type}>
                        {nodeIdTypeToLabel(type)}
                    </ToggleButton>
                ))}
            </ToggleButtonGroup>
            <TextField
                fullWidth
                error={error}
                label="Node ID"
                size="small"
                value={body}
                onChange={(e) => setBody(e.target.value)}
                disabled={isBodylessType(type)}
            />
        </Stack>
    );
};

interface ClusterIdInputProps {
    value: OptionalClusterId | undefined;
    onChange: (clusterId: OptionalClusterId) => void;
}

const ClusterIdInput: React.FC<ClusterIdInputProps> = ({ value, onChange }) => {
    const [body, setBody] = useState("0");
    const [error, setError] = useState(false);

    useEffect(() => {
        if (value !== undefined) {
            setBody(value.toHumanReadableString());
        }
    }, [value]);

    useEffect(() => {
        const result = ClusterId.schema.safeParse(body);
        setError(!result.success);
        if (result.success) {
            onChange(result.data);
        }
    }, [body, onChange]);

    return (
        <TextField
            fullWidth
            error={error}
            label="Cluster ID"
            size="small"
            value={body}
            onChange={(e) => setBody(e.target.value)}
        />
    );
};

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
                <ClusterIdInput value={clusterId} onChange={setClusterId} />
            </Grid>
        </Grid>
    );
};
