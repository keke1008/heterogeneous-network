import { Cost, RpcResult, RpcStatus } from "@core/net";
import { ClusterId, NoCluster, OptionalClusterId } from "@core/net/node/clusterId";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { useContext, useState } from "react";
import { ZodSchemaInput } from "../../Input";
import { ActionGroup, ActionRpcDialog } from "../ActionTemplates";

const SetClusterId: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [clusterId, setClusterId] = useState<OptionalClusterId>(new NoCluster());
    const setClusterIdAction = async (): Promise<RpcResult<unknown>> => {
        return net.rpc().requestSetClusterId(target, clusterId);
    };

    return (
        <ActionRpcDialog name="Set Cluster ID" onSubmit={setClusterIdAction}>
            <ZodSchemaInput<OptionalClusterId>
                schema={ClusterId.schema}
                onValue={(id) => setClusterId(id ?? new NoCluster())}
                stringValue="0"
                label="cluster id"
                type="number"
            />
        </ActionRpcDialog>
    );
};

const SetCost: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [cost, setCost] = useState<Cost>();
    const setCostAction = async (): Promise<RpcResult<unknown>> => {
        return cost === undefined ? { status: RpcStatus.BadArgument } : net.rpc().requestSetCost(target, cost);
    };

    return (
        <ActionRpcDialog name="Set Cost" onSubmit={setCostAction}>
            <ZodSchemaInput<Cost> schema={Cost.schema} onValue={setCost} stringValue="0" label="cost" type="number" />
        </ActionRpcDialog>
    );
};

export const Local: React.FC = () => {
    return (
        <ActionGroup name="Local">
            <SetClusterId />
            <SetCost />
        </ActionGroup>
    );
};
