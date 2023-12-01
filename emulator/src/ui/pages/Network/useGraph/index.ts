import { NetContext } from "@emulator/ui/contexts/netContext";
import { useAction } from "./useAction";
import { useRenderer } from "./useRenderer";
import { useContext, useEffect } from "react";

export const useGraph = (rootRef: React.RefObject<HTMLElement>) => {
    const netService = useContext(NetContext);
    const { nodeTooltip, handleClickNode, handleClickOutsideNode } = useAction();
    const { applyUpdate } = useRenderer({
        rootRef,
        onClickNode: handleClickNode,
        onClickOutsideNode: handleClickOutsideNode,
    });
    netService.onNetStateUpdate((update) => applyUpdate(update));

    useEffect(() => {
        applyUpdate(netService.syncNetState());
    }, [applyUpdate, netService]);

    return { nodeTooltip };
};
