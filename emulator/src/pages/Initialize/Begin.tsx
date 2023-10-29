import { AddressAndPort } from "../../components/AddressAndPort";

interface Props {
    onBegin: () => void;
}

export const Begin: React.FC<Props> = (props) => {
    const begin = (address: string, port: string) => {
        window.net.begin({ selfAddress: address, selfPort: port });
        props.onBegin();
    };

    return (
        <div className="connect">
            <AddressAndPort onEnter={begin} initialAddress={"127.0.0.1"} initialPort={"8080"} />
        </div>
    );
};
