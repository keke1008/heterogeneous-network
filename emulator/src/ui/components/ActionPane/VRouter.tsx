import { Destination } from "@core/net";
import { ActionGroup } from "./ActionTemplates";
import { TextField } from "@mui/material";
import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionRpcDialog } from "./ActionTemplates/ActionDialog";

interface Props {
    targetNode: Destination;
}

const GetVRouters: React.FC<Props> = ({ targetNode }) => {
    const net = useContext(NetContext);
    return <ActionRpcDialog name="Get VRouters" onSubmit={() => net.rpc().requestGetVRouters(targetNode)} />;
};

const CreateVRouter: React.FC<Props> = ({ targetNode }) => {
    const net = useContext(NetContext);
    return <ActionRpcDialog name="Create VRouter" onSubmit={() => net.rpc().requestCreateVRouter(targetNode)} />;
};

const DeleteVRouter: React.FC<Props> = ({ targetNode }) => {
    const net = useContext(NetContext);
    const [port, setPort] = useState<number>(0);

    return (
        <ActionRpcDialog name="Delete VRouter" onSubmit={() => net.rpc().requestDeleteVRouter(targetNode, port)}>
            <TextField size="small" type="number" label="port" onChange={(e) => setPort(parseInt(e.target.value))} />
        </ActionRpcDialog>
    );
};

export const VRouter: React.FC<Props> = ({ targetNode }) => {
    return (
        <ActionGroup name="VRouter">
            <GetVRouters targetNode={targetNode} />
            <CreateVRouter targetNode={targetNode} />
            <DeleteVRouter targetNode={targetNode} />
        </ActionGroup>
    );
};
