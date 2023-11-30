import { useState } from "react";
import { Initialize } from "./Initialize";
import { Network } from "./Network";
import { SelfParameter } from "./Initialize/SelfParameter";

type AppState = { type: "initialize" } | { type: "network"; selfParams: SelfParameter };

export const App: React.FC = () => {
    const [state, setState] = useState<AppState>({ type: "initialize" });

    if (state.type === "initialize") {
        return <Initialize onSubmit={(selfParams) => setState({ type: "network", selfParams })} />;
    } else if (state.type === "network") {
        return <Network selfParameter={state.selfParams} />;
    } else {
        return <div>Invalid app state</div>;
    }
};
