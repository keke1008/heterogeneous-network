import { NodeId, SerialAddress, UdpAddress, UhfAddress, WebSocketAddress } from "@core/net";
import { FormControl, Grid, MenuItem, Select, TextField } from "@mui/material";
import { NodeIdType } from "@core/net/node/nodeId";
import { useState, useEffect } from "react";
import { match } from "ts-pattern";

interface NodeIdInputProps {
    value?: NodeId;
    defaultValue?: NodeId;
    label?: string;
    onChange: (nodeId: NodeId) => void;
    nodeIdTypes?: NodeIdType[];
    inputProps?: React.ComponentProps<typeof TextField>;
    selectProps?: React.ComponentProps<typeof Select<NodeIdType>>;
}

const defaultNodeIdTypes = [
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

export const NodeIdInput: React.FC<NodeIdInputProps> = ({
    value,
    defaultValue,
    onChange,
    nodeIdTypes = defaultNodeIdTypes,
    inputProps,
    selectProps,
}) => {
    const [type, setType] = useState<NodeIdType>(nodeIdTypes[0]);
    const [body, setBody] = useState(defaultValue?.toHumanReadableBodyString() ?? "");
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
        <Grid container alignItems="end">
            <Grid item>
                <FormControl fullWidth>
                    <Select
                        {...selectProps}
                        required
                        value={type}
                        onChange={(e) => typeof e.target.value !== "string" && setType(e.target.value)}
                    >
                        {nodeIdTypes.map((type) => (
                            <MenuItem key={type} value={type}>
                                {nodeIdTypeToLabel(type)}
                            </MenuItem>
                        ))}
                    </Select>
                </FormControl>
            </Grid>
            <Grid item xs>
                <TextField
                    {...inputProps}
                    fullWidth
                    error={error}
                    value={body}
                    onChange={(e) => setBody(e.target.value)}
                    disabled={isBodylessType(type)}
                />
            </Grid>
        </Grid>
    );
};

interface ControlledNodeIdInputProps {
    type: NodeIdType;
    setType(type: NodeIdType): void;
    body: string;
    setBody(body: string): void;
    nodeIdTypes?: NodeIdType[];
    inputProps?: React.ComponentProps<typeof TextField>;
    selectProps?: React.ComponentProps<typeof Select<NodeIdType>>;
}
export const ControlledNodeIdInput: React.FC<ControlledNodeIdInputProps> = ({
    type,
    setType,
    body,
    setBody,
    nodeIdTypes = defaultNodeIdTypes,
    inputProps,
    selectProps,
}) => {
    const [error, setError] = useState(false);

    useEffect(() => {
        const nodeId = createNodeIdFromTypeAndBodyString(type, body);
        setError(nodeId === undefined);
    }, [type, body]);

    return (
        <Grid container alignItems="end">
            <Grid item>
                <FormControl fullWidth>
                    <Select
                        {...selectProps}
                        required
                        value={type}
                        onChange={(e) => typeof e.target.value !== "string" && setType(e.target.value)}
                    >
                        {nodeIdTypes.map((type) => (
                            <MenuItem key={type} value={type}>
                                {nodeIdTypeToLabel(type)}
                            </MenuItem>
                        ))}
                    </Select>
                </FormControl>
            </Grid>
            <Grid item xs>
                <TextField
                    {...inputProps}
                    fullWidth
                    error={error}
                    value={body}
                    onChange={(e) => setBody(e.target.value)}
                    disabled={isBodylessType(type)}
                />
            </Grid>
        </Grid>
    );
};
