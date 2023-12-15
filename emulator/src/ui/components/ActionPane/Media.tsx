import { NodeId } from "@core/net";
import { Action, ActionRpcButton, ActionGroup } from "./ActionTemplates";
import { useContext } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";

interface Props {
    targetId: NodeId;
}

export const Media: React.FC<Props> = ({ targetId }) => {
    const net = useContext(NetContext);
    const getMediaList = () => net.rpc().requestGetMediaList(targetId);

    return (
        <ActionGroup name="Media">
            <Action>
                <ActionRpcButton onClick={getMediaList}>Get Media List</ActionRpcButton>
            </Action>
        </ActionGroup>
    );
};