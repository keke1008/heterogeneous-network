import { AddressAndPort } from "../../components/AddressAndPort";

export const Connect: React.FC = () => {
    const connect = (address: string, port: string) => {
        window.net.connect({ address, port, cost: 0 });
    };

    return (
        <div className="connect">
            <AddressAndPort onEnter={connect} initialAddress={"127.0.0.1"} initialPort={"19073"} />
        </div>
    );
};
