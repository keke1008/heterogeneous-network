import { BlinkOperation } from "@core/net";
import { useContext } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { Stack } from "@mui/material";
import { ActionRpcButton } from "../../ActionTemplates/ActionButton";

export const Blink: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const blink = () => net.rpc().requestBlink(target, BlinkOperation.Blink);
    const stopBlinking = () => net.rpc().requestBlink(target, BlinkOperation.Stop);
    return (
        <Stack gap={4}>
            <ActionRpcButton onClick={blink} buttonProps={{ fullWidth: true }}>
                Blink
            </ActionRpcButton>
            <ActionRpcButton onClick={stopBlinking} buttonProps={{ fullWidth: true }}>
                Stop blinking
            </ActionRpcButton>
        </Stack>
    );
};
