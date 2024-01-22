import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { useContext, useState } from "react";
import { ipAddressSchema } from "./ipAddress";
import { MediaPortNumber, RpcResult, RpcStatus } from "@core/net";
import { z } from "zod";
import { ActionRpcForm } from "../../ActionTemplates";
import { ZodValidatedInput } from "@emulator/ui/components/Input";

const paramsSchema = z.object({
    mask: ipAddressSchema,
    portNumber: MediaPortNumber.schema,
});

export const SetEthernetSubnetMask: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [mask, setMask] = useState<string>("");
    const [portNumber, setPortNumber] = useState<string>("");

    const setEthernetSubnetMask = async (): Promise<RpcResult<unknown>> => {
        const params = paramsSchema.safeParse({ mask, portNumber });
        if (!params.success) {
            return { status: RpcStatus.BadArgument };
        }

        return await net.rpc().requestSetEthernetSubnetMask(target, params.data);
    };

    return (
        <ActionRpcForm onSubmit={setEthernetSubnetMask} submitButtonText="Set ethernet subnet mask">
            <ZodValidatedInput
                label="Subnet mask"
                schema={ipAddressSchema}
                value={mask}
                onChange={(e) => setMask(e.target.value)}
                fullWidth
            />
            <ZodValidatedInput
                label="Media port Number"
                type="number"
                schema={MediaPortNumber.schema}
                value={portNumber}
                onChange={(e) => setPortNumber(e.target.value)}
                fullWidth
            />
        </ActionRpcForm>
    );
};
