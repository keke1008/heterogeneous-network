import { useEffect, useState } from "react";
import { NetContext } from "./contexts/netContext";
import { NetService } from "@emulator/net/service";
import { ActionPane } from "./components/ActionPane";
import { Source } from "@core/net";
import { Grid } from "@mui/material";
import { GraphPane } from "./components/GraphPane";
import { InitializeModal } from "./components/InitializeModal";

export const App: React.FC = () => {
    const [net] = useState(() => new NetService());

    const [localnode, setLocalNode] = useState<Source | undefined>(undefined);
    const [selectedNode, setSelectedNode] = useState<Source | undefined>(undefined);

    useEffect(() => {
        net.localNode().getSource().then(setLocalNode);
    }, [net]);

    return (
        <NetContext.Provider value={net}>
            <InitializeModal />
            <Grid container direction="row" spacing={2}>
                <Grid item xs={4}>
                    <GraphPane
                        onClickNode={({ node }) => setSelectedNode(node)}
                        onClickOutsideNode={() => setSelectedNode(undefined)}
                    />
                </Grid>
                <Grid item xs={8}>
                    <ActionPane localNode={localnode} selectedNode={selectedNode} />
                </Grid>
            </Grid>
        </NetContext.Provider>
    );
};
