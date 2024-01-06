import { BlinkOperation } from "@core/net";
import { useContext } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { ActionGroup, ActionRpcDialog } from "../ActionTemplates";

export const Debug: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const blink = () => net.rpc().requestBlink(target, BlinkOperation.Blink);
    const stopBlinking = () => net.rpc().requestBlink(target, BlinkOperation.Stop);
    return (
        <ActionGroup name="Debug">
            <ActionRpcDialog name="Blink" onSubmit={blink} />
            <ActionRpcDialog name="Stop Blinking" onSubmit={stopBlinking} />
        </ActionGroup>
    );
};
