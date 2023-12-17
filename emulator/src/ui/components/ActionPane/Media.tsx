import { Destination } from "@core/net";
import { ActionGroup } from "./ActionTemplates";
import { useContext } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionRpcDialog } from "./ActionTemplates/ActionDialog";

interface Props {
    targetNode: Destination;
}

export const Media: React.FC<Props> = ({ targetNode }) => {
    const net = useContext(NetContext);
    const getMediaList = () => net.rpc().requestGetMediaList(targetNode);

    return (
        <ActionGroup name="Media">
            <ActionRpcDialog name="Get Media List" onSubmit={getMediaList} />
        </ActionGroup>
    );
};
