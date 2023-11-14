import { useEffect, useRef } from "react";
import { useGraph } from "./useGraph";
import { ipc } from "emulator/src/hooks/useIpc";

export const Graph = () => {
    const rootRef = useRef<HTMLDivElement>(null);
    const { applyUpdate } = useGraph(rootRef);

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

    return <div ref={rootRef}></div>;
};
