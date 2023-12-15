import { useContext, useState } from "react";
import { NetContext } from "../contexts/netContext";
import { NodeId } from "@core/net";
import { Button, Dialog, DialogActions, DialogContent, DialogContentText, DialogTitle } from "@mui/material";
import { NodeIdInput } from "./Input";

export const InitializeModal: React.FC = () => {
    const net = useContext(NetContext);
    const [localNodeId, setLocalNodeId] = useState<NodeId | undefined>(undefined);
    const [open, setOpen] = useState(() => net.localNode().id === undefined);

    const handleSubmit = () => {
        if (localNodeId === undefined) {
            return;
        }
        net.localNode().tryInitialize(localNodeId);
        setOpen(false);
    };

    return (
        <Dialog open={open}>
            <DialogTitle>Initialize</DialogTitle>
            <DialogContent>
                <DialogContentText>Initialize local node ID</DialogContentText>
                <NodeIdInput label="Local Node ID" onChange={setLocalNodeId} />
            </DialogContent>
            <DialogActions>
                <Button onClick={handleSubmit}>Submit</Button>
            </DialogActions>
        </Dialog>
    );
};
