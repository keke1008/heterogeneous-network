import { Box, Card, Divider, Grid, IconButton, List, ListItem, ListSubheader, Stack } from "@mui/material";
import { Gateway, Matcher, RoutingEntry } from "@vrouter/routing";
import { MatcherInput } from "./Matcher";
import { GatewayInput } from "./Gateway";
import { AddCircle, ContentCopy, DeleteForever } from "@mui/icons-material";
import { useState } from "react";

interface RoutingEntryRowLayoutProps {
    index: React.ReactNode;
    matcher: React.ReactNode;
    gateway: React.ReactNode;
    actions: React.ReactNode;
}
const RoutingEntryRowLayout: React.FC<RoutingEntryRowLayoutProps> = ({ index, matcher, gateway, actions }) => {
    return (
        <Grid container spacing={2} alignItems="center" textAlign="center">
            <Grid item xs="auto">
                {index}
            </Grid>
            <Grid item xs>
                {matcher}
            </Grid>
            <Grid item xs>
                {gateway}
            </Grid>
            <Grid item xs="auto">
                {actions}
            </Grid>
        </Grid>
    );
};

interface RoutingTableRowProps {
    index: number;
    entry: RoutingEntry;
    onCopy(): void;
    onDelete(entry: RoutingEntry): void;
}

const RoutingTableRow: React.FC<RoutingTableRowProps> = ({ index, entry, onCopy, onDelete }) => {
    return (
        <RoutingEntryRowLayout
            index={index}
            matcher={entry.matcher.display()}
            gateway={entry.gateway.display()}
            actions={
                <>
                    <IconButton onClick={onCopy} color="primary">
                        <ContentCopy />
                    </IconButton>
                    <IconButton onClick={() => onDelete(entry)} color="error">
                        <DeleteForever />
                    </IconButton>
                </>
            }
        />
    );
};

interface NewRoutingEntryInputProps {
    onAdd(entry: RoutingEntry): void;
    copiedEntry?: RoutingEntry;
}
const NewRoutingEntry: React.FC<NewRoutingEntryInputProps> = ({ onAdd, copiedEntry }) => {
    const [matcher, setMatcher] = useState<Matcher | undefined>();
    const [gateway, setGateway] = useState<Gateway | undefined>();

    const handleAdd = () => {
        if (matcher && gateway) {
            onAdd({ matcher, gateway });
        }
    };

    const addable = matcher !== undefined && gateway !== undefined;

    return (
        <Grid container justifyContent="space-between">
            <Grid item xs={5}>
                <MatcherInput copiedMatcher={copiedEntry?.matcher} onChange={setMatcher} />
            </Grid>
            <Grid item xs={5}>
                <GatewayInput copiedGateway={copiedEntry?.gateway} onChange={setGateway} />
            </Grid>
            <Grid item xs={1}>
                <IconButton onClick={handleAdd} size="large" disabled={!addable} color="primary">
                    <AddCircle fontSize="large" />
                </IconButton>
            </Grid>
        </Grid>
    );
};

const RoutingTableHeader: React.FC = () => {
    return (
        <RoutingEntryRowLayout
            index="#"
            matcher="Matcher"
            gateway="Gateway"
            actions={
                <>
                    <IconButton disabled>
                        <ContentCopy />
                    </IconButton>
                    <IconButton disabled>
                        <DeleteForever />
                    </IconButton>
                </>
            }
        />
    );
};

interface RoutingTableProps {
    entries: RoutingEntry[];
    onUpdate(entry: RoutingEntry): void;
    onDelete(entry: RoutingEntry): void;
}
export const RoutingTable: React.FC<RoutingTableProps> = ({ entries, onUpdate, onDelete }) => {
    const [copiedEntry, setCopiedEntry] = useState<RoutingEntry | undefined>();
    const handleCopy = (entry: RoutingEntry) => {
        setCopiedEntry({ matcher: entry.matcher.clone(), gateway: entry.gateway.clone() });
    };

    return (
        <Stack spacing={1}>
            <List
                subheader={
                    <ListSubheader>
                        <RoutingTableHeader />
                    </ListSubheader>
                }
            >
                <Divider />
                {entries.map((entry, index) => (
                    <ListItem key={entry.matcher.uniqueKey()}>
                        <RoutingTableRow
                            index={index}
                            entry={entry}
                            onCopy={() => handleCopy(entry)}
                            onDelete={onDelete}
                        />
                    </ListItem>
                ))}
            </List>

            <Box>
                <Card sx={{ padding: 2, marginTop: 4 }}>
                    <NewRoutingEntry copiedEntry={copiedEntry} onAdd={onUpdate} />
                </Card>
            </Box>
        </Stack>
    );
};
