import "@fontsource/roboto/300.css";
import "@fontsource/roboto/400.css";
import "@fontsource/roboto/500.css";
import "@fontsource/roboto/700.css";

import "./style.css";

import React from "react";
import ReactDOM from "react-dom/client";
import { CssBaseline, ThemeProvider, createTheme } from "@mui/material";
import { RouterProvider, createBrowserRouter } from "react-router-dom";
import { App } from "./ui/App";
import { NetworkAction } from "./ui/components/ActionPane/NetworkAction";
import { routes as networkActionRoutes } from "./ui/components/ActionPane/NetworkAction/actions";
import { AppAction } from "./ui/components/ActionPane/AppAction";
import { Echo } from "./ui/components/ActionPane/AppAction/Echo";
import { Caption } from "./ui/components/ActionPane/AppAction/Caption";
import { FileAction } from "./ui/components/ActionPane/AppAction/File";

const theme = createTheme({
    palette: { mode: "dark" },
});

const router = createBrowserRouter([
    {
        path: "/",
        element: <App />,
        children: [
            { path: "network", Component: NetworkAction, children: networkActionRoutes },
            {
                path: "apps",
                element: <AppAction />,
                children: [
                    { path: "echo", element: <Echo /> },
                    { path: "caption", element: <Caption /> },
                    { path: "file", element: <FileAction /> },
                ],
            },
        ],
    },
]);

ReactDOM.createRoot(document.getElementById("root")!).render(
    <React.StrictMode>
        <ThemeProvider theme={theme}>
            <CssBaseline />
            <RouterProvider router={router} />
        </ThemeProvider>
    </React.StrictMode>,
);
