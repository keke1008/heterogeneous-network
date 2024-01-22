import { RpcResult } from "@core/net";
import { ClusterId, NoCluster, OptionalClusterId } from "@core/net/node/clusterId";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { useContext, useState } from "react";
import { ZodSchemaInput } from "@emulator/ui/components/Input";
import { ActionRpcForm } from "../../ActionTemplates";

export const SetClusterId: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [clusterId, setClusterId] = useState<OptionalClusterId>(new NoCluster());
    const setClusterIdAction = async (): Promise<RpcResult<unknown>> => {
        return net.rpc().requestSetClusterId(target, clusterId);
    };

    return (
        <ActionRpcForm onSubmit={setClusterIdAction} submitButtonText="Set cluster ID">
            <ZodSchemaInput<OptionalClusterId>
                schema={ClusterId.schema}
                onValue={(id) => setClusterId(id ?? new NoCluster())}
                stringValue="0"
                label="cluster id"
                type="number"
                fullWidth
            />
        </ActionRpcForm>
    );
};
