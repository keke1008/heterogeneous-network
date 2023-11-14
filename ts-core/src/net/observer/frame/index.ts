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
import { InvalidNotifyContentError, NotifyFrame, deserializeNotifyFrame } from "./notify";
import { Result } from "oxide.ts";
import { AddressError } from "@core/net/link";
import { FrameType, InvalidNotifyFrameTypeError, deserializeFrameType } from "./common";

export type ObserverFrame = NotifyFrame | SubscribeFrame;

export type DeserializeFrameError = AddressError | InvalidNotifyContentError | InvalidNotifyFrameTypeError;

export const deserializeObserverFrame = (reader: BufferReader): Result<ObserverFrame, DeserializeFrameError> => {
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
