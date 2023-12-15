import { BlinkOperation, NodeId } from "@core/net";
import { Action, ActionButton, ActionGroup } from "./ActionTemplates";
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
                <ActionButton onClick={blink}>Blink</ActionButton>
                <ActionButton onClick={stopBlinking}>Stop Blinking</ActionButton>
            </Action>
        </ActionGroup>
    );
};
