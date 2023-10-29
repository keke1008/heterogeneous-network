import { useState } from "react";

export interface Props {
    onEnter: (address: string, port: string) => void;
    initialAddress?: string;
    initialPort?: string;
}

export const AddressAndPort: React.FC<Props> = ({ onEnter, initialAddress, initialPort }) => {
    const [address, setAddress] = useState<string>(initialAddress || "");
    const [port, setPort] = useState<string>(initialPort || "");

    return (
        <div>
            <div>
                <label htmlFor="address">Address</label>
                <input type="text" placeholder="Address" value={address} onChange={(e) => setAddress(e.target.value)} />
            </div>

            <div>
                <label htmlFor="port">Port</label>
                <input type="text" placeholder="Port" value={port} onChange={(e) => setPort(e.target.value)} />
            </div>
            <button onClick={() => onEnter(address, port)}>Enter</button>
        </div>
    );
};
