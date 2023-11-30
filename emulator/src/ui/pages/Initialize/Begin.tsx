import { SelfParameter } from "./SelfParameter";

interface Props {
    onSubmit: (params: SelfParameter) => void;
}

export const Begin: React.FC<Props> = (props) => {
    const handleSubmit = (data: SelfParameter) => {
        props.onSubmit(data);
    };

    return (
        <div className="connect">
            <SelfParameter onSubmit={handleSubmit} initialUdpAddress="127.0.0.1:8000" initialSerialAddress="42" />
        </div>
    );
};
