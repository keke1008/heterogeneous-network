import { NodeId } from "@core/net";
import { Action, ActionButton, ActionGroup } from "./ActionTemplates";
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
                <ActionButton onClick={getMediaList}>Get Media List</ActionButton>
            </Action>
        </ActionGroup>
    );
};
