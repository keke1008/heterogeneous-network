import { NodeId, NodeIdType } from "@core/net/node/nodeId";
import { ControlledNodeIdInput } from "@emulator/ui/components/Input";
import { FormControl, Grid, InputLabel, MenuItem, Select } from "@mui/material";
import { DiscoveryGateway, Gateway, GatewayType, UnicastGateway } from "@vrouter/routing";
import { useEffect, useState } from "react";

interface GatewayTypeInputProps {
    gatewayType: GatewayType;
    onChange(gatewayType: GatewayType): void;
}
const GatewayTypeInput: React.FC<GatewayTypeInputProps> = ({ gatewayType, onChange }) => {
    const gatewayTypes = [GatewayType.Unicast, GatewayType.Discovery, GatewayType.Discard];
    return (
        <FormControl fullWidth>
            <InputLabel>Gateway</InputLabel>
            <Select
                required
                variant="standard"
                value={gatewayType}
                onChange={(e) => typeof e.target.value !== "string" && onChange(e.target.value)}
            >
                {gatewayTypes.map((type) => (
                    <MenuItem key={type} value={type}>
                        {GatewayType[type]}
                    </MenuItem>
                ))}
            </Select>
        </FormControl>
    );
};

interface GatewayBodyInputProps {
    copiedGateway?: Gateway;
    onChange(gateway?: Gateway): void;
}
const UnicastGatewayInput: React.FC<GatewayBodyInputProps> = ({ copiedGateway, onChange }) => {
    const nodeIdTypes = [NodeIdType.Serial, NodeIdType.UHF, NodeIdType.IPv4, NodeIdType.WebSocket];
    const [type, setType] = useState<NodeIdType>(nodeIdTypes[0]);
    const [body, setBody] = useState("");

    useEffect(() => {
        const result = NodeId.schema.safeParse({ type, body });
        if (result.success) {
            onChange(new UnicastGateway(result.data));
        }
    }, [type, body, onChange]);

    useEffect(() => {
        if (copiedGateway instanceof UnicastGateway) {
            onChange(copiedGateway);
            setType(copiedGateway.nodeId.type());
            setBody(copiedGateway.nodeId.toHumanReadableBodyString());
        }
    }, [copiedGateway, onChange]);

    return (
        <ControlledNodeIdInput
            type={type}
            setType={setType}
            body={body}
            setBody={setBody}
            nodeIdTypes={nodeIdTypes}
            inputProps={{ variant: "standard", label: "Node ID" }}
            selectProps={{ variant: "standard" }}
        />
    );
};

const DiscoveryGatewayInput: React.FC<GatewayBodyInputProps> = ({ onChange }) => {
    useEffect(() => {
        onChange(new DiscoveryGateway());
    }, [onChange]);

    return <></>;
};

const DiscardGatewayInput: React.FC<GatewayBodyInputProps> = ({ onChange }) => {
    useEffect(() => {
        onChange(undefined);
    }, [onChange]);

    return <></>;
};

interface GatewayInputProps {
    copiedGateway?: Gateway;
    onChange(gateway?: Gateway): void;
}
export const GatewayInput: React.FC<GatewayInputProps> = ({ copiedGateway, onChange }) => {
    const [gatewayType, setGatewayType] = useState(copiedGateway?.type ?? GatewayType.Unicast);
    useEffect(() => {
        if (copiedGateway?.type !== undefined) {
            setGatewayType(copiedGateway.type);
        }
    }, [copiedGateway]);

    return (
        <Grid container spacing={2}>
            <Grid item>
                <GatewayTypeInput gatewayType={gatewayType} onChange={setGatewayType} />
            </Grid>
            <Grid item xs>
                {gatewayType === GatewayType.Unicast && (
                    <UnicastGatewayInput copiedGateway={copiedGateway} onChange={onChange} />
                )}
                {gatewayType === GatewayType.Discovery && (
                    <DiscoveryGatewayInput copiedGateway={copiedGateway} onChange={onChange} />
                )}
                {gatewayType === GatewayType.Discard && (
                    <DiscardGatewayInput copiedGateway={copiedGateway} onChange={onChange} />
                )}
            </Grid>
        </Grid>
    );
};
