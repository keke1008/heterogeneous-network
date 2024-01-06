export type { ActionResult } from "./useActionButton";

import { RpcResult, RpcStatus } from "@core/net";
import { ActionResult, useActionButton } from "./useActionButton";
import { LoadingButton } from "@mui/lab";

interface Props {
    onClick?: () => Promise<ActionResult>;
    children?: React.ReactNode;
    buttonProps?: React.ComponentProps<typeof LoadingButton>;
}

export const ActionButton: React.FC<Props> = ({ onClick, children, buttonProps }) => {
    const {
        children: buttonChildren,
        color,
        loading,
        startAction,
    } = useActionButton({
        defaultChildren: children,
        action: async () => {
            try {
                return onClick ? await onClick() : { type: "success" };
            } catch (e) {
                console.error(e);
                return { type: "failure", reason: `${e}` };
            }
        },
    });

    return (
        <LoadingButton
            {...buttonProps}
            fullWidth
            variant="outlined"
            color={color}
            loading={loading}
            onClick={startAction}
        >
            {buttonChildren}
        </LoadingButton>
    );
};

interface RpcProps {
    name: string;
    onClick: () => Promise<RpcResult<unknown>>;
    children?: React.ReactNode | React.ReactNode[];
}

const rpcStatusToReason = (status: RpcStatus): string => {
    return RpcStatus[status].replace(/([A-Z])/g, " $1").trim();
};

export const ActionRpcButton: React.FC<RpcProps> = ({ onClick, children }) => {
    const handleClick = async (): Promise<ActionResult> => {
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
    };

    return <ActionButton onClick={handleClick}>{children}</ActionButton>;
};
