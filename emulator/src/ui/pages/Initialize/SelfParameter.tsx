import { SerialAddress, UdpAddress } from "@core/net";
import { Result } from "oxide.ts/core";
import { useState } from "react";

export interface SelfParameter {
    udpAddress: UdpAddress;
    serialAddress: SerialAddress;
}

interface Props {
    onSubmit: (data: SelfParameter) => void;
    initialUdpAddress?: string;
    initialSerialAddress?: string;
}

export const SelfParameter: React.FC<Props> = (props) => {
    const { onSubmit, initialUdpAddress, initialSerialAddress } = props;
    const [udpAddress, setUdpAddress] = useState(initialUdpAddress ?? "");
    const [serialAddress, setSerialAddress] = useState(initialSerialAddress ?? "");

    const handleSubmit = (e: React.FormEvent<HTMLFormElement>) => {
        e.preventDefault();

        Result.all(UdpAddress.fromHumanReadableString(udpAddress), SerialAddress.fromString(serialAddress)).map(
            ([udpAddress, serialAddress]) => onSubmit({ udpAddress, serialAddress }),
        );
    };

    return (
        <form onSubmit={handleSubmit}>
            <div>
                <label htmlFor="udpAddress">UDP Address</label>
                <input type="text" id="udpAddress" value={udpAddress} onChange={(e) => setUdpAddress(e.target.value)} />
            </div>
            <div>
                <label htmlFor="serialAddress">Serial Address</label>
                <input
                    type="text"
                    id="serialAddress"
                    value={serialAddress}
                    onChange={(e) => setSerialAddress(e.target.value)}
                />
            </div>
            <button type="submit" className="btn btn-primary">
                Submit
            </button>
        </form>
    );
};
