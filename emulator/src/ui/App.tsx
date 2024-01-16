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

    return (
        <NetContext.Provider value={net}>
            <InitializeModal />
            <Grid container direction="row" spacing={8} paddingLeft={1} height="100%">
                <Grid item xs={5}>
                    <GraphPane selectedDestination={selected} onSelectedDestinationChange={setSelected} />
                </Grid>
                <Grid item xs={7}>
                    {selected && (
                        <ActionContext.Provider value={{ target: selected }}>
                            <ActionPane />
                        </ActionContext.Provider>
                    )}
                </Grid>
            </Grid>
        </NetContext.Provider>
    );
};
