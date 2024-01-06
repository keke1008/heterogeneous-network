import { ActionGroup } from "./ActionTemplates";
import { TextField } from "@mui/material";
import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionRpcDialog } from "./ActionTemplates/ActionDialog";
import { ActionContext } from "@emulator/ui/contexts/actionContext";

const GetVRouters: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    return <ActionRpcDialog name="Get VRouters" onSubmit={() => net.rpc().requestGetVRouters(target)} />;
};

const CreateVRouter: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    return <ActionRpcDialog name="Create VRouter" onSubmit={() => net.rpc().requestCreateVRouter(target)} />;
};

const DeleteVRouter: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);
    const [port, setPort] = useState<number>(0);

    return (
        <ActionRpcDialog name="Delete VRouter" onSubmit={() => net.rpc().requestDeleteVRouter(target, port)}>
            <TextField size="small" type="number" label="port" onChange={(e) => setPort(parseInt(e.target.value))} />
        </ActionRpcDialog>
    );
};

export const VRouter: React.FC = () => {
    return (
        <ActionGroup name="VRouter">
            <GetVRouters />
            <CreateVRouter />
            <DeleteVRouter />
        </ActionGroup>
    );
};
