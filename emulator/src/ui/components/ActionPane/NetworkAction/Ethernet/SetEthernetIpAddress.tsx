import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { useContext, useState } from "react";
import { ipAddressSchema } from "./ipAddress";
import { MediaPortNumber, RpcResult, RpcStatus } from "@core/net";
import { z } from "zod";
import { ActionRpcForm } from "../../ActionTemplates";
import { ZodValidatedInput } from "@emulator/ui/components/Input";

const paramsSchema = z.object({
    ipAddress: ipAddressSchema,
    portNumber: MediaPortNumber.schema,
});

export const SetEthernetIpAddress: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [ipAddress, setIpAddress] = useState<string>("");
    const [portNumber, setPortNumber] = useState<string>("");

    const setEthernetIpAddress = async (): Promise<RpcResult<unknown>> => {
        const params = paramsSchema.safeParse({ ipAddress, portNumber });
        if (!params.success) {
            return { status: RpcStatus.BadArgument };
        }

        return await net.rpc().requestSetEthernetIpAddress(target, params.data);
    };

    return (
        <ActionRpcForm onSubmit={setEthernetIpAddress} submitButtonText="Set ethernet ip address">
            <ZodValidatedInput
                label="IP address"
                schema={ipAddressSchema}
                value={ipAddress}
                onChange={(e) => setIpAddress(e.target.value)}
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
