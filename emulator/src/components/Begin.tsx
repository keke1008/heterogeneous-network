import { useState } from "react";

interface Props {
    onBegin: () => void;
}

export const Begin: React.FC<Props> = (props) => {
    const [address, setAddress] = useState("127.0.0.1");
    const [port, setPort] = useState("8080");

    const begin = () => {
        window.net.begin({ selfAddress: address, selfPort: port });
        props.onBegin();
    };

    return (
        <div className="connect">
            <div>
                <label htmlFor="address">Address</label>
                <input type="text" id="address" value={address} onChange={(e) => setAddress(e.target.value)} />
            </div>
            <div>
                <label htmlFor="port">Port</label>
                <input type="text" id="port" value={port} onChange={(e) => setPort(e.target.value)} />
            </div>
            <div>
                <button onClick={begin}>Begin</button>
            </div>
        </div>
    );
};
