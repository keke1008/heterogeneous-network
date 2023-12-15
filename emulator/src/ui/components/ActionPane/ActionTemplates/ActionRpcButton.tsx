import { useActionButton } from "./useActionButton";
import { Grid } from "@mui/material";
import { LoadingButton } from "@mui/lab";
import { RpcResult, RpcStatus } from "@core/net";

interface Props {
    onClick: () => Promise<RpcResult<unknown>>;
    children: React.ReactNode;
}

const rpcStatusToReason = (status: RpcStatus): string => {
    return RpcStatus[status].replace(/([A-Z])/g, " $1").trim();
};

export const ActionRpcButton: React.FC<Props> = ({ onClick, children }) => {
    const {
        children: buttonChildren,
        color,
        loading,
        startAction,
    } = useActionButton({
        defaultChildren: children,
        action: async () => {
            try {
                const result = await onClick();
                console.info(result);
                return result.status === RpcStatus.Success
                    ? { type: "success" }
                    : { type: "failure", reason: rpcStatusToReason(result.status) };
            } catch (e) {
                console.error(e);
                return { type: "failure", reason: "Unknown error" };
            }
        },
    });

    return (
        <Grid item xs={3}>
            <LoadingButton fullWidth variant="outlined" color={color} loading={loading} onClick={startAction}>
                {buttonChildren}
            </LoadingButton>
        </Grid>
    );
};
