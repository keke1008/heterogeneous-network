import { Begin } from "./Begin";

export interface Props {
    onInitialized: () => void;
}

export const Initialize: React.FC<Props> = (props) => {
    return (
        <>
            <h1>Initialize</h1>
            <Begin onBegin={props.onInitialized} />
        </>
    );
};
