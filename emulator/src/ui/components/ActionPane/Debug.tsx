import { BlinkOperation, NodeId } from "@core/net";
import { Action, ActionRpcButton, ActionGroup } from "./ActionTemplates";
import { useContext } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";

interface Props {
    targetId: NodeId;
}

export const Debug: React.FC<Props> = ({ targetId }) => {
    const net = useContext(NetContext);
    const blink = () => net.rpc().requestBlink(targetId, BlinkOperation.Blink);
    const stopBlinking = () => net.rpc().requestBlink(targetId, BlinkOperation.Stop);
    return (
        <ActionGroup name="Debug">
            <Action>
                <ActionRpcButton onClick={blink}>Blink</ActionRpcButton>
                <ActionRpcButton onClick={stopBlinking}>Stop Blinking</ActionRpcButton>
            </Action>
        </ActionGroup>
    );
};
