import { Graph } from "./Graph";
import { ConnectSerial } from "./ConnectSerial";
import { ConnectWebSocket } from "./ConnectWebSocket";

export const Network: React.FC = () => {
    return (
        <>
            <Graph />
            <ConnectSerial />
            <ConnectWebSocket />
        </>
    );
};
