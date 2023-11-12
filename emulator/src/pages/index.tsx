import { useState } from "react";
import { Initialize } from "./Initialize";
import { Network } from "./Network";

type AppState = "initialize" | "network";

export const App: React.FC = () => {
    const [state, setState] = useState<AppState>("initialize");
    if (state === "initialize") {
        return <Initialize onInitialized={() => setState("network")} />;
    } else if (state === "network") {
        return <Network />;
    } else {
        return <div>Invalid app state</div>;
    }
};
