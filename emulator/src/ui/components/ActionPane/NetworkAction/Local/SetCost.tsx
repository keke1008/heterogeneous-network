import { Cost, RpcResult, RpcStatus } from "@core/net";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { useContext, useState } from "react";
import { ZodSchemaInput } from "../../../Input";
import { ActionRpcForm } from "../../ActionTemplates";

export const SetCost: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [cost, setCost] = useState<Cost>();
    const setCostAction = async (): Promise<RpcResult<unknown>> => {
        return cost === undefined ? { status: RpcStatus.BadArgument } : net.rpc().requestSetCost(target, cost);
    };

    return (
        <ActionRpcForm onSubmit={setCostAction} submitButtonText="Set cost">
            <ZodSchemaInput<Cost>
                schema={Cost.schema}
                onValue={setCost}
                stringValue="0"
                label="cost"
                type="number"
                fullWidth
            />
        </ActionRpcForm>
    );
};
