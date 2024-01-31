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
import { actions as appActionRoutes } from "./ui/components/ActionPane/AppAction/actions";
import { AppAction } from "./ui/components/ActionPane/AppAction";

const theme = createTheme({
    palette: { mode: "dark" },
});

const router = createBrowserRouter([
    {
        path: "/",
        element: <App />,
        children: [
            { path: "network", Component: NetworkAction, children: networkActionRoutes },
            { path: "apps", Component: AppAction, children: appActionRoutes },
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
