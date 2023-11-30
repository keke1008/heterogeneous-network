import { Cost, UdpAddress } from "@core/net";
import { ipc } from "emulator/src/hooks/useIpc";
import { Result } from "oxide.ts/core";
import { useState } from "react";

export const ConnectUdp: React.FC = () => {
    const connectUdp = ipc.net.connectUdp.useInvoke();
    const [address, setaddress] = useState("");
    const [cost, setCost] = useState(0);

    const connect = () => {
        const deserializedAddress = UdpAddress.fromHumanReadableString(address);
        const deserializedCost = Cost.fromNumber(cost);
        Result.all(deserializedAddress, deserializedCost).map(([address, cost]) => connectUdp({ address, cost }));
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
