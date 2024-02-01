import { ClusterId } from "@core/net";
import { NodeId, NodeIdType } from "@core/net/node/nodeId";
import { ControlledNodeIdInput } from "@emulator/ui/components/Input";
import { ControlledClusterIdInput } from "@emulator/ui/components/Input/ClusterIdInput";
import { FormControl, Grid, InputLabel, MenuItem, Select } from "@mui/material";
import { ClusterIdMatcher, DefaultMatcher, Matcher, MatcherType, NodeIdMatcher } from "@vrouter/routing";
import { useEffect, useState } from "react";

interface MatcherTypeInputProps {
    matcherType: MatcherType;
    onChange(matcherType: MatcherType): void;
}
const MatcherTypeInput: React.FC<MatcherTypeInputProps> = ({ matcherType, onChange }) => {
    const matcherTypes = [MatcherType.NodeId, MatcherType.ClusterId, MatcherType.Default];
    return (
        <FormControl fullWidth>
            <InputLabel>Matcher</InputLabel>
            <Select
                required
                variant="standard"
                value={matcherType}
                onChange={(e) => typeof e.target.value !== "string" && onChange(e.target.value)}
            >
                {matcherTypes.map((type) => (
                    <MenuItem key={type} value={type}>
                        {MatcherType[type]}
                    </MenuItem>
                ))}
            </Select>
        </FormControl>
    );
};

interface MatcherBodyInputProps {
    copiedMatcher?: Matcher;
    onChange(matcher?: Matcher): void;
}
const NodeIdMatcherInput: React.FC<MatcherBodyInputProps> = ({ copiedMatcher, onChange }) => {
    const nodeIdTypes = [NodeIdType.Serial, NodeIdType.UHF, NodeIdType.IPv4, NodeIdType.WebSocket];
    const [type, setType] = useState<NodeIdType>(nodeIdTypes[0]);
    const [body, setBody] = useState("");

    useEffect(() => {
        const result = NodeId.schema.safeParse({ type, body });
        if (result.success) {
            onChange(new NodeIdMatcher(result.data));
        } else {
            onChange(undefined);
        }
    }, [type, body, onChange]);

    useEffect(() => {
        if (copiedMatcher instanceof NodeIdMatcher) {
            onChange(copiedMatcher);
            setType(copiedMatcher.nodeId.type());
            setBody(copiedMatcher.nodeId.toHumanReadableBodyString());
        } else {
            onChange(undefined);
        }
    }, [copiedMatcher, onChange]);

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

const ClusterIdMatcherInput: React.FC<MatcherBodyInputProps> = ({ copiedMatcher, onChange }) => {
    const [body, setBody] = useState("");

    useEffect(() => {
        const clusterId = ClusterId.noClusterExcludedSchema.safeParse(body);
        if (clusterId.success) {
            onChange(new ClusterIdMatcher(clusterId.data));
        } else {
            onChange(undefined);
        }
    }, [body, onChange]);

    useEffect(() => {
        if (copiedMatcher instanceof ClusterIdMatcher) {
            onChange(copiedMatcher);
            setBody(copiedMatcher.clusterId.toHumanReadableString());
        }
    }, [copiedMatcher, onChange]);

    return (
        <ControlledClusterIdInput
            value={body}
            onChange={setBody}
            inputProps={{ variant: "standard", label: "Cluster ID" }}
        />
    );
};

const DefaultMatcherInput: React.FC<MatcherBodyInputProps> = ({ onChange }) => {
    useEffect(() => {
        onChange(new DefaultMatcher());
    }, [onChange]);

    return <></>;
};

interface MatcherInputProps {
    copiedMatcher?: Matcher;
    onChange(matcher: Matcher): void;
}
export const MatcherInput: React.FC<MatcherInputProps> = ({ copiedMatcher, onChange }) => {
    const [matcherType, setMatcherType] = useState(copiedMatcher?.type ?? MatcherType.NodeId);
    useEffect(() => {
        if (copiedMatcher) {
            setMatcherType(copiedMatcher.type);
        }
    }, [copiedMatcher]);

    return (
        <Grid container spacing={2}>
            <Grid item>
                <MatcherTypeInput matcherType={matcherType} onChange={setMatcherType} />
            </Grid>
            <Grid item xs>
                {matcherType === MatcherType.NodeId && (
                    <NodeIdMatcherInput copiedMatcher={copiedMatcher} onChange={onChange} />
                )}
                {matcherType === MatcherType.ClusterId && (
                    <ClusterIdMatcherInput copiedMatcher={copiedMatcher} onChange={onChange} />
                )}
                {matcherType === MatcherType.Default && (
                    <DefaultMatcherInput copiedMatcher={copiedMatcher} onChange={onChange} />
                )}
            </Grid>
        </Grid>
    );
};
