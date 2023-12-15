import { RpcResult, RpcStatus } from "@core/net";
import { LoadingButton } from "@mui/lab";
import { Grid } from "@mui/material";
import { useEffect, useState } from "react";
import { P, match } from "ts-pattern";

interface Props {
    onClick: () => Promise<RpcResult<unknown>>;
    children: React.ReactNode;
}

const rpcStatusToReason = (status: RpcStatus): string => {
    return RpcStatus[status].replace(/([A-Z])/g, " $1").trim();
};

type State = { type: "idle" } | { type: "loading" } | { type: "success" } | { type: "failure"; reason: string };

const FEEDBACK_STATE_DURATION_MS = 2000;

export const ActionButton: React.FC<Props> = ({ onClick, children }) => {
    const [state, setState] = useState<State>({ type: "idle" });
    useEffect(() => {
        if (state.type === "success" || state.type === "failure") {
            const timeout = setTimeout(() => setState({ type: "idle" }), FEEDBACK_STATE_DURATION_MS);
            return () => clearTimeout(timeout);
        }
    }, [state]);

    const handleClick = async () => {
        setState({ type: "loading" });
        try {
            const result = await onClick();
            console.info(result);
            if (result.status === RpcStatus.Success) {
                setState({ type: "success" });
            } else {
                setState({ type: "failure", reason: rpcStatusToReason(result.status) });
            }
        } catch (e) {
            console.error(e);
            setState({ type: "failure", reason: "Unknown error" });
        }
    };

    const buttonChildren = match(state)
        .with({ type: P.union("idle", "loading") }, () => children)
        .with({ type: "success" }, () => "Success")
        .with({ type: "failure" }, ({ reason }) => reason)
        .exhaustive();

    const color = match(state)
        .with({ type: P.union("idle", "loading") }, () => undefined)
        .with({ type: "success" }, () => "success" as const)
        .with({ type: "failure" }, () => "error" as const)
        .exhaustive();

    return (
        <Grid item xs={3}>
            <LoadingButton
                fullWidth
                color={color}
                variant="outlined"
                loading={state.type === "loading"}
                onClick={handleClick}
            >
                {buttonChildren}
            </LoadingButton>
        </Grid>
    );
};
