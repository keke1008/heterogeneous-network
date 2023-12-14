import "@fontsource/roboto/300.css";
import "@fontsource/roboto/400.css";
import "@fontsource/roboto/500.css";
import "@fontsource/roboto/700.css";

import React from "react";
import ReactDOM from "react-dom/client";
import { App } from "./ui/pages";
import { ActionTab } from "./ui/components/ActionTab";
import { NodeId } from "@core/net";

ReactDOM.createRoot(document.getElementById("root")!).render(
    <React.StrictMode>
        <App />
        <ActionTab localNodeId={NodeId.loopback()} selectedNodeId={undefined} />
    </React.StrictMode>,
);
