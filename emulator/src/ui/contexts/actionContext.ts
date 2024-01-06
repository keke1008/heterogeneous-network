import { Destination } from "@core/net";
import { createContext } from "react";
import { createDummy } from "./dummy";

export type ActionContextValue = {
    target: Destination;
};

export const ActionContext = createContext<ActionContextValue>(createDummy("ActionContext"));
