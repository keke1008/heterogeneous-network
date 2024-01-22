import { Grid } from "@mui/material";
import { RpcResult } from "@core/net";
import React from "react";
import { ActionButton, ActionResult, ActionRpcButton } from "./ActionButton";

interface ActionFormProps {
    children: React.ReactNode;
    submitButtonText: string;
    onSubmit: () => Promise<ActionResult>;
}

export const ActionForm: React.FC<ActionFormProps> = ({ children, onSubmit, submitButtonText }) => {
    const childrenArray = React.Children.toArray(children);

    return (
        <Grid container spacing={2}>
            {childrenArray.map((child, index) => (
                <Grid item xs={12} key={index}>
                    {child}
                </Grid>
            ))}

            <Grid item xs={12} marginTop={4}>
                <ActionButton onClick={onSubmit}>{submitButtonText}</ActionButton>
            </Grid>
        </Grid>
    );
};

interface ActionRpcFormProps {
    children: React.ReactNode;
    submitButtonText: string;
    submitButtonProps?: React.ComponentProps<typeof ActionRpcButton>;
    onSubmit: () => Promise<RpcResult<unknown>>;
}

export const ActionRpcForm: React.FC<ActionRpcFormProps> = ({
    children,
    onSubmit,
    submitButtonText,
    submitButtonProps,
}) => {
    const childrenArray = React.Children.toArray(children);

    return (
        <Grid container spacing={2}>
            {childrenArray.map((child, index) => (
                <Grid item xs={12} key={index}>
                    {child}
                </Grid>
            ))}

            <Grid item xs={12} marginTop={4}>
                <ActionRpcButton onClick={onSubmit} buttonProps={{ fullWidth: true, ...submitButtonProps }}>
                    {submitButtonText}
                </ActionRpcButton>
            </Grid>
        </Grid>
    );
};
