import { useEffect, useState } from "react";
import { P, match } from "ts-pattern";

export type ActionButtonState =
    | { type: "idle" }
    | { type: "loading" }
    | { type: "success" }
    | { type: "failure"; reason: string };

export type ActionResult = { type: "success" } | { type: "failure"; reason: string };

const FEEDBACK_STATE_DURATION_MS = 2000;

interface Props {
    defaultChildren: React.ReactNode;
    action: () => Promise<ActionResult>;
}

export const useActionButton = ({ defaultChildren, action }: Props) => {
    const [state, setState] = useState<ActionButtonState>({ type: "idle" });

    useEffect(() => {
        if (state.type === "success" || state.type === "failure") {
            const timeout = setTimeout(() => setState({ type: "idle" }), FEEDBACK_STATE_DURATION_MS);
            return () => clearTimeout(timeout);
        }
    }, [state, setState]);

    const loading = state.type === "loading";

    const children = match(state)
        .with({ type: P.union("idle", "loading") }, () => defaultChildren)
        .with({ type: "success" }, () => "Success")
        .with({ type: "failure" }, ({ reason }) => reason)
        .exhaustive();

    const color = match(state)
        .with({ type: P.union("idle", "loading") }, () => undefined)
        .with({ type: "success" }, () => "success" as const)
        .with({ type: "failure" }, () => "error" as const)
        .exhaustive();

    const startAction = async () => {
        setState({ type: "loading" });
        setState(await action());
    };

    return { children, color, loading, startAction };
};
