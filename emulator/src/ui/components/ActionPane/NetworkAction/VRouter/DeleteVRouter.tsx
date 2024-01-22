import { TextField } from "@mui/material";
import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { ActionRpcForm } from "../../ActionTemplates";

export const DeleteVRouter: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);
    const [port, setPort] = useState<number>(0);

    return (
        <ActionRpcForm onSubmit={() => net.rpc().requestDeleteVRouter(target, port)} submitButtonText="Delete vrouter">
            <TextField
                size="small"
                type="number"
                label="port"
                fullWidth
                onChange={(e) => setPort(parseInt(e.target.value))}
            />
        </ActionRpcForm>
    );
};
