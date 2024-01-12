import { useEffect, useState } from "react";
import { NetContext } from "./contexts/netContext";
import { NetService } from "@emulator/net/service";
import { ActionPane } from "./components/ActionPane";
import { ClusterId, Destination, PartialNode, Source } from "@core/net";
import { Grid } from "@mui/material";
import { GraphPane } from "./components/GraphPane";
import { InitializeModal } from "./components/InitializeModal";
import { ActionContext } from "./contexts/actionContext";
import { AppServer } from "@emulator/apps";

export const App: React.FC = () => {
    const [net] = useState(() => new NetService());

    const [local, setLocal] = useState<Source>();
    useEffect(() => {
        net.localNode().getSource().then(setLocal);
    }, [net]);

    const [apps] = useState(() => new AppServer({ trustedService: net.trusted() }));
    useEffect(() => {
        apps.startEchoServer();
        apps.startCaptionServer();
    }, [apps]);

    const [selected, setSelected] = useState<PartialNode>();
    const target =
        selected !== undefined
            ? new Destination({ nodeId: selected.nodeId, clusterId: selected.clusterId ?? ClusterId.noCluster() })
            : local?.intoDestination();

    return (
        <NetContext.Provider value={net}>
            <InitializeModal />
            <Grid container direction="row" spacing={2}>
                <Grid item xs={6}>
                    <GraphPane
                        onClickNode={(node) => setSelected(node)}
                        onClickOutsideNode={() => setSelected(undefined)}
                    />
                </Grid>
                <Grid item xs={6}>
                    {target && (
                        <ActionContext.Provider value={{ target }}>
                            <ActionPane />
                        </ActionContext.Provider>
                    )}
                </Grid>
            </Grid>
        </NetContext.Provider>
    );
};
