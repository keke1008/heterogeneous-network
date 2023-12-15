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
    const { applyUpdates, resize } = useGraphRenderer({ rootRef, onClickNode, onClickOutsideNode });

    useEffect(() => {
        applyUpdates(netService.dumpNetworkStateAsUpdate());
        return netService.onNetworkUpdate((update) => applyUpdates(update));
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    useEffect(() => {
        resize({ width: 800, height: 800 });
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    return <Box ref={rootRef} sx={{ border: 2, borderRadius: 1, borderColor: "primary.main" }}></Box>;
};
