import { Begin } from "./Begin";
import { SelfParameter } from "./SelfParameter";

export interface Props {
    onSubmit: (param: SelfParameter) => void;
}

export const Initialize: React.FC<Props> = (props) => {
    return (
        <>
            <h1>Initialize</h1>
            <Begin onSubmit={props.onSubmit} />
        </>
    );
};
