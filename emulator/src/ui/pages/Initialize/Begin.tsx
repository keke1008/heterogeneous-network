import { InitializeParameter } from "@emulator/net/service";
import { SelfParameter } from "./SelfParameter";

interface Props {
    onSubmit: (params: InitializeParameter) => void;
}

export const Begin: React.FC<Props> = (props) => {
    return (
        <div className="connect">
            <SelfParameter onSubmit={props.onSubmit} initialSerialAddress={"42"} />
        </div>
    );
};
