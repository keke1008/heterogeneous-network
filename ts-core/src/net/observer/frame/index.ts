export { FrameType } from "./common";
export {
    SelfUpdatedFrame,
    NeighborRemovedFrame,
    NeighborUpdatedFrame,
    NodeSubscriptionFrame,
    createNodeNotificationFrameFromLocalNotification,
} from "./node";
export type { NodeFrame, NodeNotificationFrame } from "./node";
export {
    NetworkNodeUpdatedNotificationEntry,
    NetworkNodeRemovedNotificationEntry,
    NetworkLinkUpdatedNotificationEntry,
    NetworkLinkRemovedNotificationEntry,
    NetworkSubscriptionFrame,
    NetworkNotificationFrame,
    NetworkUpdate,
} from "./network";
export type { NetworkNotificationEntry, NetworkFrame } from "./network";

import { BufferReader } from "@core/net/buffer";
import { FrameType, deserializeFrameType } from "./common";
import { DeserializeResult } from "@core/serde";
import { NodeFrame, deserializeNodeFrame } from "./node";
import { NetworkFrame, deserializeNetworkFrame } from "./network";

export type ObserverFrame = NodeFrame | NetworkFrame;

export const deserializeObserverFrame = (reader: BufferReader): DeserializeResult<ObserverFrame> => {
    return deserializeFrameType(reader).andThen<ObserverFrame>((frameType) => {
        switch (frameType) {
            case FrameType.NodeSubscription:
            case FrameType.NodeNotification:
                return deserializeNodeFrame(frameType, reader);
            case FrameType.NetworkSubscription:
            case FrameType.NetworkNotification:
                return deserializeNetworkFrame(frameType, reader);
        }
    });
};
