import { Graph } from "./Graph";
import { ConnectSerial } from "./ConnectSerial";
import { ConnectUdp } from "./ConnectUdp";

export const Network: React.FC = () => {
    return (
        <>
            <Graph />
            <ConnectSerial />
            <ConnectUdp />
        </>
    );
};
