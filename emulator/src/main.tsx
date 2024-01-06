import "@fontsource/roboto/300.css";
import "@fontsource/roboto/400.css";
import "@fontsource/roboto/500.css";
import "@fontsource/roboto/700.css";

import React from "react";
import ReactDOM from "react-dom/client";
import { CssBaseline, ThemeProvider, createTheme } from "@mui/material";
import { RouterProvider, createBrowserRouter } from "react-router-dom";
import { App } from "./ui/App";
import { NetworkAction } from "./ui/components/ActionPane/NetworkAction";
import { AppAction } from "./ui/components/ActionPane/AppAction";
import { Echo } from "./ui/components/ActionPane/AppAction/Echo";

const theme = createTheme({
    palette: { mode: "dark" },
});

const router = createBrowserRouter([
    {
        path: "/",
        element: <App />,
        children: [
            { index: true, element: <NetworkAction /> },
            {
                path: "apps",
                element: <AppAction />,
                children: [{ path: "echo", element: <Echo /> }],
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
