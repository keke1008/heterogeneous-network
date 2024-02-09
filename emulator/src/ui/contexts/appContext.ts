import { AppServer } from "@emulator/apps";
import { createContext } from "react";
import { createDummy } from "./dummy";

export const AppContext = createContext<AppServer>(createDummy("AppServer"));
