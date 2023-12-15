import { ActionResult, useActionButton } from "./useActionButton";
import { Grid } from "@mui/material";
import { LoadingButton } from "@mui/lab";

interface Props {
    onClick: () => Promise<ActionResult>;
    children: React.ReactNode;
}

export const ActionButton: React.FC<Props> = ({ onClick, children }) => {
    const {
        children: buttonChildren,
        color,
        loading,
        startAction,
    } = useActionButton({
        defaultChildren: children,
        action: async () => {
            try {
                return await onClick();
            } catch (e) {
                console.error(e);
                return { type: "failure", reason: `${e}` };
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
