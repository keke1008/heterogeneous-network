import { Box, Dialog, DialogActions, DialogContent, DialogTitle, Grid, IconButton, Stack } from "@mui/material";
import { useState } from "react";
import CloseIcon from "@mui/icons-material/Close";
import { LoadingButton } from "@mui/lab";
import { ActionResult } from "./useActionButton";
import { ActionButton } from ".";
import { RpcResult, RpcStatus } from "@core/net";

interface Props {
    children?: React.ReactNode | React.ReactNode[];
    name: string;
    onSubmit: () => Promise<ActionResult>;
}

export const ActionDialog: React.FC<Props> = ({ children, name, onSubmit }) => {
    const [open, setOpen] = useState(false);
    const handleClose = () => setOpen(false);

    const childrenArray = Array.isArray(children) ? children : [children];

    return (
        <>
            <Grid item xs={3}>
                <LoadingButton fullWidth variant="outlined" color="primary" onClick={() => setOpen(true)}>
                    {name}
                </LoadingButton>
            </Grid>
            <Dialog open={open} onClose={handleClose}>
                <DialogTitle>
                    <Box display="flex" justifyContent="space-between" alignItems="center" gap={4}>
                        <Box>{name}</Box>
                        <IconButton
                            aria-label="close"
                            onClick={handleClose}
                            sx={{ padding: 0, color: (theme) => theme.palette.grey[500] }}
                        >
                            <CloseIcon />
                        </IconButton>
                    </Box>
                </DialogTitle>
                <DialogContent>
                    <Stack spacing={4} marginY={2}>
                        {childrenArray.map((child, index) => (
                            <Grid key={index} item>
                                {child}
                            </Grid>
                        ))}
                    </Stack>
                </DialogContent>
                <DialogActions>
                    <ActionButton onClick={onSubmit}>Submit</ActionButton>
                </DialogActions>
            </Dialog>
        </>
    );
};

interface RpcDialogProps {
    name: string;
    onSubmit: () => Promise<RpcResult<unknown>>;
    children?: React.ReactNode | React.ReactNode[];
}

const rpcStatusToReason = (status: RpcStatus): string => {
    return RpcStatus[status].replace(/([A-Z])/g, " $1").trim();
};

export const ActionRpcDialog: React.FC<RpcDialogProps> = ({ name, onSubmit, children }) => {
    const handleSubmit = async () => {
        try {
            const result = await onSubmit();
            console.info(result);
            return result.status === RpcStatus.Success
                ? ({ type: "success" } as const)
                : ({ type: "failure", reason: rpcStatusToReason(result.status) } as const);
        } catch (e) {
            console.error(e);
            return { type: "failure", reason: "Unknown error" } as const;
        }
    };

    return (
        <ActionDialog name={name} onSubmit={handleSubmit}>
            {children}
        </ActionDialog>
    );
};
