import { useEffect, useState } from "react";
import { NetContext } from "./contexts/netContext";
import { NetService } from "@emulator/net/service";
import { ActionPane } from "./components/ActionPane";
import { Source } from "@core/net";
import { Grid } from "@mui/material";
import { GraphPane } from "./components/GraphPane";
import { InitializeModal } from "./components/InitializeModal";
import { ActionContext } from "./contexts/actionContext";

export const App: React.FC = () => {
    const [net] = useState(() => new NetService());

    const [local, setLocal] = useState<Source>();
    useEffect(() => {
        net.localNode().getSource().then(setLocal);
    }, [net]);

    const [selected, setSelected] = useState<Source>();
    const target = selected?.intoDestination() ?? local?.intoDestination();

    return (
        <NetContext.Provider value={net}>
            <InitializeModal />
            <Grid container direction="row" spacing={2}>
                <Grid item xs={4}>
                    <GraphPane
                        onClickNode={({ node }) => setSelected(node)}
                        onClickOutsideNode={() => setSelected(undefined)}
                    />
                </Grid>
                <Grid item xs={8}>
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
