import { Begin } from "./Begin";

export interface Props {
    onInitialized: () => void;
}

export const Initialize: React.FC<Props> = (props) => {
    return <Begin onBegin={props.onInitialized} />;
};
