import { NetService } from "emulator/src/net/service";
import { createContext } from "react";
import { createDummy } from "./dummy";

export const NetContext = createContext<NetService>(createDummy("NetContext"));
