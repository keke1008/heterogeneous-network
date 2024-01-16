import { useMemo, useRef } from "react";
import { useGraphRenderer } from "./useGraphRenderer";
import { Box, Stack } from "@mui/material";
import { useContext, useEffect } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { useGraphControl } from "./useGraphControl";
import { ClusterId, Destination } from "@core/net";
import { DestinationInput } from "../Input";
import { useTheme } from "@mui/material";

interface Props {
    onSelectedDestinationChange: (destination: Destination | undefined) => void;
    selectedDestination: Destination | undefined;
}

export const GraphPane: React.FC<Props> = ({ selectedDestination, onSelectedDestinationChange }) => {
    const netService = useContext(NetContext);
    const rootRef = useRef<HTMLDivElement>(null);

    const theme = useTheme();
    const colorPalette = useMemo(
        () => ({
            nodeDefault: theme.palette.primary.main,
            nodeSelected: theme.palette.secondary.main,
            nodeReceived: theme.palette.error.main,
            nodeText: theme.palette.text.primary,
            text: theme.palette.text.primary,
            link: theme.palette.text.primary,
        }),
        [theme],
    );

    const { rendererRef } = useGraphRenderer({ rootRef, colorPalette });

    useEffect(() => {
        return rendererRef.current?.onClickNode((node) => {
            const clusterId = node.clusterId ?? ClusterId.noCluster();
            onSelectedDestinationChange(new Destination({ nodeId: node.nodeId, clusterId }));
        });
    }, [netService, onSelectedDestinationChange, rendererRef]);

    const { applyNetworkUpdates } = useGraphControl({ graphRef: rendererRef, selectedDestination, colorPalette });
    useEffect(() => {
        applyNetworkUpdates(netService.dumpNetworkStateAsUpdate());
        return netService.onNetworkTopologyUpdate((update) => applyNetworkUpdates(update));
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    return (
        <Stack>
            <Box
                ref={rootRef}
                sx={{
                    border: 2,
                    borderRadius: 1,
                    borderColor: "primary.main",
                    aspectRatio: "1 / 1",
                }}
            />
            <DestinationInput value={selectedDestination} onChange={onSelectedDestinationChange} />
        </Stack>
    );
};
