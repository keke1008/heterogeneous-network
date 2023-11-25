export { FrameType } from "./common";
export {
    InvalidNotifyContentError,
    LinkRemovedFrame,
    NotifyContent,
    LinkUpdatedFrame,
    NodeRemovedFrame,
    NodeUpdatedFrame,
    fromNotification,
} from "./notify";
export type { NotifyFrame } from "./notify";
export { SubscribeFrame } from "./subscribe";

import { SubscribeFrame } from "./subscribe";
import { BufferReader } from "@core/net/buffer";
import { NotifyFrame, deserializeNotifyFrame } from "./notify";
import { FrameType, deserializeFrameType } from "./common";
import { DeserializeResult } from "@core/serde";

export type ObserverFrame = NotifyFrame | SubscribeFrame;

export const deserializeObserverFrame = (reader: BufferReader): DeserializeResult<ObserverFrame> => {
    const type = deserializeFrameType(reader);
    if (type.isErr()) {
        return type;
    }

    switch (type.unwrap()) {
        case FrameType.Subscribe:
            return SubscribeFrame.deserialize(reader);
        case FrameType.Notify:
            return deserializeNotifyFrame(reader);
    }
};
