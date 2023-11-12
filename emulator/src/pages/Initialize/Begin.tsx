import { ipc } from "emulator/src/hooks/useIpc";
import { Parameter, SelfParameter } from "./SelfParameter";

interface Props {
    onBegin: () => void;
}

export const Begin: React.FC<Props> = (props) => {
    const begin = ipc.net.begin.useInvoke({ onSuccess: () => props.onBegin() });

    const handleSubmit = (data: Parameter) => {
        begin({ selfUdpAddress: data.udpAddress, selfSerialAddress: data.serialAddress });
    };

    return (
        <div className="connect">
            <SelfParameter onSubmit={handleSubmit} />
        </div>
    );
};
