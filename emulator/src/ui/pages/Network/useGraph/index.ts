import { NetContext } from "@emulator/ui/contexts/netContext";
import { useAction } from "./useAction";
import { useRenderer } from "./useRenderer";
import { useContext, useEffect } from "react";

export const useGraph = (rootRef: React.RefObject<HTMLElement>) => {
    const netService = useContext(NetContext);
    const { nodeTooltip, handleClickNode, handleClickOutsideNode } = useAction();
    const { applyUpdates } = useRenderer({
        rootRef,
        onClickNode: handleClickNode,
        onClickOutsideNode: handleClickOutsideNode,
    });

    useEffect(() => {
        applyUpdates(netService.dumpNetworkStateAsUpdate());
        return netService.onNetworkUpdate((update) => applyUpdates(update));
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    return { nodeTooltip };
};
