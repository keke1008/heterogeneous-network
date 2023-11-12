import { useRef } from "react";
import { useGraph } from "./useGraph";
import { ipc } from "emulator/src/hooks/useIpc";

export const Graph = () => {
    const rootRef = useRef<HTMLDivElement>(null);
    const { applyModification } = useGraph(rootRef);
    ipc.net.onGraphModified.useListen((modfiication) => applyModification(modfiication));

    return <div ref={rootRef}></div>;
};
