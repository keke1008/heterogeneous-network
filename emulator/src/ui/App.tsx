import { useEffect, useState } from "react";
import { NetContext } from "./contexts/netContext";
import { NetService } from "@emulator/net/service";
import { ActionPane } from "./components/ActionPane";
import { NodeId } from "@core/net";
import { Grid } from "@mui/material";
import { GraphPane } from "./components/GraphPane";
import { InitializeModal } from "./components/InitializeModal";

export const App: React.FC = () => {
    const [net] = useState(() => new NetService());

    const [localnodeId, setLocalNodeId] = useState<NodeId | undefined>(undefined);
    const [selectedNodeId, setSelectedNodeId] = useState<NodeId | undefined>(undefined);

    useEffect(() => {
        net.localNode().getId().then(setLocalNodeId);
    }, [net]);

    return (
        <NetContext.Provider value={net}>
            <InitializeModal />
            <Grid container direction="row" spacing={2}>
                <Grid item xs={4}>
                    <GraphPane
                        onClickNode={({ nodeId }) => setSelectedNodeId(nodeId)}
                        onClickOutsideNode={() => setSelectedNodeId(undefined)}
                    />
                </Grid>
                <Grid item xs={8}>
                    <ActionPane localNodeId={localnodeId} selectedNodeId={selectedNodeId} />
                </Grid>
            </Grid>
        </NetContext.Provider>
    );
};
