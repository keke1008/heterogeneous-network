import { NetService } from "emulator/src/net/service";
import { createContext } from "react";

const dummyValue = new Proxy({} as NetService, {
    get: () => {
        throw new Error("NetContext not initialized");
    },
});

export const NetContext = createContext<NetService>(dummyValue);
