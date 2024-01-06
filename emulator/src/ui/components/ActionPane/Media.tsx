import { ActionGroup } from "./ActionTemplates";
import { useContext } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionRpcDialog } from "./ActionTemplates/ActionDialog";
import { ActionContext } from "@emulator/ui/contexts/actionContext";

export const Media: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);
    const getMediaList = () => net.rpc().requestGetMediaList(target);

    return (
        <ActionGroup name="Media">
            <ActionRpcDialog name="Get Media List" onSubmit={getMediaList} />
        </ActionGroup>
    );
};
