import { useEffect, useState } from "react";
import { NetContext } from "./contexts/netContext";
import { NetService } from "@emulator/net/service";
import { ActionPane } from "./components/ActionPane";
import { Destination } from "@core/net";
import { Grid } from "@mui/material";
import { GraphPane } from "./components/GraphPane";
import { InitializeModal } from "./components/InitializeModal";
import { ActionContext } from "./contexts/actionContext";
import { AppServer } from "@emulator/apps";
import { AppContext } from "./contexts/appContext";

export const App: React.FC = () => {
    const [selected, setSelected] = useState<Destination>();

    const [net] = useState(() => new NetService());
    useEffect(() => {
        net.localNode()
            .getInfo()
            .then((info) => setSelected(info.source.intoDestination()));
    }, [net]);

    const [apps] = useState(() => new AppServer({ trustedService: net.trusted(), streamService: net.stream() }));
    useEffect(() => {
        apps.startEchoServer();
        apps.startCaptionServer();
        apps.startFileServer();
        apps.startAiImageServer();
    }, [apps]);

    return (
        <NetContext.Provider value={net}>
            <AppContext.Provider value={apps}>
                <InitializeModal />
                <Grid container height="100%">
                    <Grid item xs={5} height="100%">
                        <GraphPane selectedDestination={selected} onSelectedDestinationChange={setSelected} />
                    </Grid>
                    <Grid item xs paddingLeft={2} height="100%">
                        {selected && (
                            <ActionContext.Provider value={{ target: selected }}>
                                <ActionPane />
                            </ActionContext.Provider>
                        )}
                    </Grid>
                </Grid>
            </AppContext.Provider>
        </NetContext.Provider>
    );
};
