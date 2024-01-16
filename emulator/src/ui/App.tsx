import { useEffect, useState } from "react";
import { NetContext } from "./contexts/netContext";
import { NetService } from "@emulator/net/service";
import { ActionPane } from "./components/ActionPane";
import { Destination } from "@core/net";
import { Box, CssBaseline, Grid, IconButton, ThemeProvider, createTheme } from "@mui/material";
import { GraphPane } from "./components/GraphPane";
import { InitializeModal } from "./components/InitializeModal";
import { ActionContext } from "./contexts/actionContext";
import { AppServer } from "@emulator/apps";
import { Brightness4 } from "@mui/icons-material";

export const App: React.FC = () => {
    const [selected, setSelected] = useState<Destination>();

    const [net] = useState(() => new NetService());
    useEffect(() => {
        net.localNode()
            .getInfo()
            .then((info) => setSelected(info.source.intoDestination()));
    }, [net]);

    const [apps] = useState(() => new AppServer({ trustedService: net.trusted() }));
    useEffect(() => {
        apps.startEchoServer();
        apps.startCaptionServer();
    }, [apps]);

    const [darkMode, setDarkMode] = useState(true);
    const theme = createTheme({
        palette: { mode: darkMode ? "dark" : "light" },
    });

    return (
        <ThemeProvider theme={theme}>
            <CssBaseline />
            <NetContext.Provider value={net}>
                <InitializeModal />
                <Grid container direction="row" spacing={4} height="100%">
                    <Grid item xs={5}>
                        <Box margin={2}>
                            <GraphPane selectedDestination={selected} onSelectedDestinationChange={setSelected} />
                        </Box>
                    </Grid>
                    <Grid item xs>
                        {selected && (
                            <ActionContext.Provider value={{ target: selected }}>
                                <ActionPane />
                            </ActionContext.Provider>
                        )}
                    </Grid>
                    <Grid item>
                        <IconButton edge="start" onClick={() => setDarkMode(!darkMode)}>
                            <Brightness4 />
                        </IconButton>
                    </Grid>
                </Grid>
            </NetContext.Provider>
        </ThemeProvider>
    );
};
