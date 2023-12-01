import { SerialAddress } from "@core/net";
import { InitializeParameter } from "@emulator/net/service";
import { useState } from "react";

interface Props {
    onSubmit: (data: InitializeParameter) => void;
    initialSerialAddress?: string;
}

export const SelfParameter: React.FC<Props> = (props) => {
    const { onSubmit, initialSerialAddress } = props;
    const [serialAddress, setSerialAddress] = useState(initialSerialAddress ?? "");

    const handleSubmit = (e: React.FormEvent<HTMLFormElement>) => {
        e.preventDefault();

        SerialAddress.fromString(serialAddress).map((serialAddress) => onSubmit({ localSerialAddress: serialAddress }));
    };

    return (
        <form onSubmit={handleSubmit}>
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
