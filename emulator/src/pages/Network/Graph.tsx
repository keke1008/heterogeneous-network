import { useRef } from "react";
import { useGraph } from "./useGraph";
import { ipc } from "emulator/src/hooks/useIpc";

export const Graph = () => {
    const rootRef = useRef<HTMLDivElement>(null);
    const { applyUpdate } = useGraph(rootRef);
    ipc.net.onNetStateUpdate.useListen((update) => applyUpdate(update));

    return <div ref={rootRef}></div>;
};
