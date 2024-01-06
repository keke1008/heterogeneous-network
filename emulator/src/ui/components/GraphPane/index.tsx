import { useRef } from "react";
import { useGraphRenderer } from "./useGraphRenderer";
import { Box } from "@mui/material";
import { useContext, useEffect } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { useGraphControl } from "./useGraphControl";
import { Source } from "@core/net";

interface Props {
    onClickNode?: (node: Source) => void;
    onClickOutsideNode?: () => void;
}

export const GraphPane: React.FC<Props> = ({ onClickNode, onClickOutsideNode }) => {
    const netService = useContext(NetContext);
    const rootRef = useRef<HTMLDivElement>(null);
    const { rendererRef } = useGraphRenderer({ rootRef });
    const { applyNetworkUpdates } = useGraphControl(rendererRef);

    useEffect(() => {
        const cancel1 = rendererRef.current?.onClickNode(onClickNode ?? (() => {}));
        const cancel2 = rendererRef.current?.onClickOutsideNode(onClickOutsideNode ?? (() => {}));
        return () => {
            cancel1?.();
            cancel2?.();
        };
    }, [onClickNode, onClickOutsideNode, rendererRef]);

    useEffect(() => {
        applyNetworkUpdates(netService.dumpNetworkStateAsUpdate());
        return netService.onNetworkTopologyUpdate((update) => applyNetworkUpdates(update));
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
