import { Graph } from "./Graph";
import { ConnectSerial } from "./ConnectSerial";
import { ConnectUdp } from "./ConnectUdp";
import { SelfParameter } from "../Initialize/SelfParameter";
import { ipc } from "emulator/src/hooks/useIpc";
import { useEffect } from "react";

interface Props {
    selfParameter: SelfParameter;
}

export const Network: React.FC<Props> = ({ selfParameter }: Props) => {
    const begin = ipc.net.begin.useInvoke();
    const end = ipc.net.end.useInvoke();

    useEffect(() => {
        begin({
            localUdpAddress: selfParameter.udpAddress,
            localSerialAddress: selfParameter.serialAddress,
        });

        return () => {
            end();
        };
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    return (
        <>
            <Graph />
            <ConnectSerial />
            <ConnectUdp />
        </>
    );
};
