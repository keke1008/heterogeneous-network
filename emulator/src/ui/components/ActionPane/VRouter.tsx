import { NodeId } from "@core/net";
import { Action, ActionButton, ActionGroup, ActionParameter } from "./ActionTemplates";
import { TextField } from "@mui/material";
import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";

interface Props {
    targetId: NodeId;
}

const GetVRouters: React.FC<Props> = ({ targetId }) => {
    const net = useContext(NetContext);
    return (
        <Action>
            <ActionButton onClick={() => net.rpc().requestGetVRouters(targetId)}>Get VRouters</ActionButton>
        </Action>
    );
};

const CreateVRouter: React.FC<Props> = ({ targetId }) => {
    const net = useContext(NetContext);
    return (
        <Action>
            <ActionButton onClick={() => net.rpc().requestCreateVRouter(targetId)}>Create VRouter</ActionButton>
        </Action>
    );
};

const DeleteVRouter: React.FC<Props> = ({ targetId }) => {
    const net = useContext(NetContext);
    const [port, setPort] = useState<number>(0);

    return (
        <Action>
            <ActionButton onClick={() => net.rpc().requestDeleteVRouter(targetId, port)}>Delete VRouter</ActionButton>
            <ActionParameter>
                <TextField
                    size="small"
                    type="number"
                    label="port"
                    onChange={(e) => setPort(parseInt(e.target.value))}
                />
            </ActionParameter>
        </Action>
    );
};

export const VRouter: React.FC<Props> = ({ targetId }) => {
    return (
        <ActionGroup name="VRouter">
            <GetVRouters targetId={targetId} />
            <CreateVRouter targetId={targetId} />
            <DeleteVRouter targetId={targetId} />
        </ActionGroup>
    );
};
