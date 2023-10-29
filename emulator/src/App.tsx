import { useState } from "react";
import { Graph } from "./components/Graph";
import { Begin } from "./components/Begin";

export const App = () => {
    const [state, setState] = useState<"begin" | "graph">("begin");
    if (state === "begin") {
        return <Begin onBegin={() => setState("graph")} />;
    } else {
        return <Graph />;
    }
};
