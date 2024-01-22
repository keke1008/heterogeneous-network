import { ActionRpcButton } from "../../ActionTemplates";
import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { Alert, Stack } from "@mui/material";
import { RpcStatus } from "@core/net";

export const CreateVRouter: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [createdVRouterPort, setCreatedVRouterPort] = useState<number>();

    const createVRouter = async () => {
        const result = await net.rpc().requestCreateVRouter(target);
        if (result.status === RpcStatus.Success) {
            setCreatedVRouterPort(result.value.port);
        } else {
            setCreatedVRouterPort(undefined);
        }
        return result;
    };

    return (
        <Stack spacing={2}>
            <ActionRpcButton onClick={createVRouter}>Create VRouter</ActionRpcButton>

            {createdVRouterPort !== undefined && (
                <Alert severity="success">Created VRouter port: {createdVRouterPort}</Alert>
            )}
        </Stack>
    );
};
