import { ipc } from "emulator/src/hooks/useIpc";
import { useAction } from "./useAction";
import { useRenderer } from "./useRenderer";
import { useEffect } from "react";

export const useGraph = (rootRef: React.RefObject<HTMLElement>) => {
    const { nodeTooltip, handleClickNode, handleClickOutsideNode } = useAction();
    const { applyUpdate } = useRenderer({
        rootRef,
        onClickNode: handleClickNode,
        onClickOutsideNode: handleClickOutsideNode,
    });
    ipc.net.onNetStateUpdate.useListen((update) => applyUpdate(update));
    const syncNetState = ipc.net.syncNetState.useInvoke();

    useEffect(() => {
        let ignore = false;
        syncNetState().then((update) => ignore || applyUpdate(update));
        return () => {
            ignore = true;
        };
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    return { nodeTooltip };
};
