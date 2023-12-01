import { Cost, WebSocketAddress } from "@core/net";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { Result } from "oxide.ts/core";
import { useContext, useState } from "react";

export const ConnectWebSocket: React.FC = () => {
    const netService = useContext(NetContext);
    const [address, setaddress] = useState("");
    const [cost, setCost] = useState(0);

    const connect = () => {
        const deserializedAddress = WebSocketAddress.fromHumanReadableString(address);
        const deserializedCost = Cost.fromNumber(cost);
        Result.all(deserializedAddress, deserializedCost).map(([remoteAddress, linkCost]) =>
            netService.connectWebSocket({ remoteAddress, linkCost }),
        );
    };

    return (
        <div className="connectUdp">
            <h2>Connect UDP</h2>
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
