import { BlinkOperation, Destination } from "@core/net";
import { ActionGroup } from "./ActionTemplates";
import { useContext } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionRpcDialog } from "./ActionTemplates/ActionDialog";

interface Props {
    targetNode: Destination;
}

export const Debug: React.FC<Props> = ({ targetNode }) => {
    const net = useContext(NetContext);
    const blink = () => net.rpc().requestBlink(targetNode, BlinkOperation.Blink);
    const stopBlinking = () => net.rpc().requestBlink(targetNode, BlinkOperation.Stop);
    return (
        <ActionGroup name="Debug">
            <ActionRpcDialog name="Blink" onSubmit={blink} />
            <ActionRpcDialog name="Stop Blinking" onSubmit={stopBlinking} />
        </ActionGroup>
    );
};
