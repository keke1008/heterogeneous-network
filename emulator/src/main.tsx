import "@fontsource/roboto/300.css";
import "@fontsource/roboto/400.css";
import "@fontsource/roboto/500.css";
import "@fontsource/roboto/700.css";

import React from "react";
import ReactDOM from "react-dom/client";
import { RouterProvider, createBrowserRouter } from "react-router-dom";
import { App } from "./ui/App";
import { NetworkAction } from "./ui/components/ActionPane/NetworkAction";
import { AppAction } from "./ui/components/ActionPane/AppAction";
import { Echo } from "./ui/components/ActionPane/AppAction/Echo";
import { Caption } from "./ui/components/ActionPane/AppAction/Caption";

const router = createBrowserRouter([
    {
        path: "/",
        element: <App />,
        children: [
            { index: true, element: <NetworkAction /> },
            {
                path: "apps",
                element: <AppAction />,
                children: [
                    { path: "echo", element: <Echo /> },
                    { path: "caption", element: <Caption /> },
                ],
            },
        ],
    },
]);

ReactDOM.createRoot(document.getElementById("root")!).render(
    <React.StrictMode>
        <RouterProvider router={router} />
    </React.StrictMode>,
);
