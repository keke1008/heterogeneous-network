import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { Address, AddressType, Cost, SerialAddress, WebSocketAddress } from "@core/net";
import { Action, ActionButton, ActionGroup, ActionParameter } from "./ActionTemplates";
import { AddressInput } from "../Input";
import { ActionResult } from "./ActionTemplates/useActionButton";
import { Result } from "oxide.ts/core";
import { ZodSchemaInput } from "../Input/ZodSchemaInput";

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
            <Action>
                <ActionButton onClick={handleConnect}>Connect</ActionButton>
                <ActionParameter>
                    <AddressInput
                        label="remote address"
                        onChange={setAddress}
                        types={[AddressType.Serial, AddressType.WebSocket]}
                        initialType={AddressType.WebSocket}
                        stringValue="127.0.0.1:12346"
                    />
                </ActionParameter>
                <ActionParameter>
                    <ZodSchemaInput<Cost>
                        schema={Cost.schema}
                        onValue={setCost}
                        stringValue="0"
                        label="link cost"
                        type="number"
                    />
                </ActionParameter>
            </Action>
        </ActionGroup>
    );
};
