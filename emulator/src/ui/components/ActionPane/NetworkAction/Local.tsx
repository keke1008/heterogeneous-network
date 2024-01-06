import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { Address, AddressType, Cost, SerialAddress, WebSocketAddress } from "@core/net";
import { ActionDialog, ActionGroup, ActionResult } from "../ActionTemplates";
import { Result } from "oxide.ts/core";
import { AddressInput, ZodSchemaInput } from "@emulator/ui/components/Input";

export const Local: React.FC = () => {
    const net = useContext(NetContext);
    const [address, setAddress] = useState<Address | undefined>(undefined);
    const [cost, setCost] = useState<Cost | undefined>();

    const handleConnect = async (): Promise<ActionResult> => {
        if (address === undefined || cost === undefined) {
            return { type: "failure", reason: "Invalid address or cost" };
        }

        const intoActionResult = (result: Result<void, unknown>): ActionResult => {
            return result.isOk() ? { type: "success" } : { type: "failure", reason: `${result.unwrapErr()}` };
        };

        const addr = address.address;
        if (addr instanceof SerialAddress) {
            const result = await net.connectSerial({ remoteAddress: addr, linkCost: cost });
            return intoActionResult(result);
        } else if (addr instanceof WebSocketAddress) {
            const result = await net.connectWebSocket({ remoteAddress: addr, linkCost: cost });
            return intoActionResult(result);
        } else {
            throw new Error("Invalid address type");
        }
    };

    return (
        <ActionGroup name="Local">
            <ActionDialog name="Connect" onSubmit={handleConnect}>
                <AddressInput
                    label="remote address"
                    onValue={setAddress}
                    types={[AddressType.WebSocket, AddressType.Serial]}
                    stringValue="127.0.0.1:12346"
                />
                <ZodSchemaInput<Cost>
                    schema={Cost.schema}
                    onValue={setCost}
                    stringValue="0"
                    label="link cost"
                    type="number"
                />
            </ActionDialog>
        </ActionGroup>
    );
};
