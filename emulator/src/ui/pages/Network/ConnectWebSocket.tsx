import { Cost, WebSocketAddress } from "@core/net";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { Result } from "oxide.ts/core";
import { useContext, useState } from "react";

export const ConnectWebSocket: React.FC = () => {
    const netService = useContext(NetContext);
    const [address, setaddress] = useState("127.0.0.1:12346");
    const [cost, setCost] = useState(0);

    const connect = () => {
        const deserializedAddress = WebSocketAddress.fromHumanReadableString(address);
        const deserializedCost = Cost.fromNumber(cost);
        const result = Result.all(deserializedAddress, deserializedCost).map(([remoteAddress, linkCost]) =>
            netService.connectWebSocket({ remoteAddress, linkCost }),
        );
        if (result.isErr()) {
            console.warn(result.unwrapErr());
        }
    };

    return (
        <div className="connectWebSocket">
            <h2>Connect WebSocket</h2>
            <div>
                <label htmlFor="address">Address</label>
                <input type="text" id="address" value={address} onChange={(e) => setaddress(e.target.value)} />
            </div>
            <div>
                <label htmlFor="cost">Cost</label>
                <input type="number" id="cost" value={cost} onChange={(e) => setCost(Number(e.target.value))} />
            </div>
            <button type="button" className="btn btn-primary" onClick={connect}>
                Connect
            </button>
        </div>
    );
};
