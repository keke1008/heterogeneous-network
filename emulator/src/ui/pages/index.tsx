import React, { useReducer } from "react";
import { Initialize } from "./Initialize";
import { Network } from "./Network";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { InitializeParameter, NetService } from "@emulator/net/service";

const netReducer: React.Reducer<NetService, InitializeParameter> = (state, action) => {
    state.registerFrameHandler(action);
    return state;
};

export const App: React.FC = () => {
    const [netService, dispatchNetService] = useReducer(netReducer, undefined, () => new NetService());

    return (
        <NetContext.Provider value={netService}>
            <Initialize onSubmit={dispatchNetService} />;
            <Network />
        </NetContext.Provider>
    );
};
