import { InitializeParameter } from "@emulator/net/service";
import { Begin } from "./Begin";

export interface Props {
    onSubmit: (param: InitializeParameter) => void;
}

export const Initialize: React.FC<Props> = (props) => {
    return (
        <>
            <h1>Initialize</h1>
            <Begin onSubmit={props.onSubmit} />
        </>
    );
};
