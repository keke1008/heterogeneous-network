import React, { useReducer } from "react";
import { Initialize } from "./Initialize";
import { Network } from "./Network";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { InitializeParameter, NetService } from "@emulator/net/service";

const netReducer: React.Reducer<NetService | undefined, InitializeParameter> = (state, action) => {
    state?.end();
    return new NetService(action);
};

export const App: React.FC = () => {
    const [netService, dispatchNetService] = useReducer(netReducer, undefined);

    if (netService === undefined) {
        return <Initialize onSubmit={dispatchNetService} />;
    } else {
        return (
            <NetContext.Provider value={netService}>
                <Network />;
            </NetContext.Provider>
        );
    }
};
