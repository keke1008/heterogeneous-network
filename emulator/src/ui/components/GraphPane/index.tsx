import { useRef } from "react";
import { NodeClickEvent, useGraphRenderer } from "./useGraphRenderer";
import { Box } from "@mui/material";
import { useContext, useEffect } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";

interface Props {
    onClickNode?: (event: NodeClickEvent) => void;
    onClickOutsideNode?: () => void;
}

export const GraphPane: React.FC<Props> = ({ onClickNode, onClickOutsideNode }) => {
    const netService = useContext(NetContext);
    const rootRef = useRef<HTMLDivElement>(null);
    const { applyUpdates } = useGraphRenderer({ rootRef, onClickNode, onClickOutsideNode });

    useEffect(() => {
        applyUpdates(netService.dumpNetworkStateAsUpdate());
        return netService.onNetworkTopologyUpdate((update) => applyUpdates(update));
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    return (
        <Box
            ref={rootRef}
            sx={{
                border: 2,
                borderRadius: 1,
                borderColor: "primary.main",
                aspectRatio: "1 / 1",
            }}
        ></Box>
    );
};
