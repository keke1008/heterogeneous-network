export { FrameType } from "./common";
export {
    SelfUpdatedEntry,
    NeighborRemovedEntry,
    NeighborUpdatedEntry,
    NodeNotificationEntry,
    NodeNotificationFrame,
    NodeSubscriptionFrame,
    NodeSyncFrame,
    FrameReceivedEntry,
} from "./node";
export type { NodeFrame } from "./node";
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

import { VariantSerdeable } from "@core/serde";
import { NodeFrame, NodeNotificationFrame, NodeSubscriptionFrame, NodeSyncFrame } from "./node";
import { NetworkFrame, NetworkNotificationFrame, NetworkSubscriptionFrame } from "./network";

export type ObserverFrame = NodeFrame | NetworkFrame;

export const ObserverFrame = {
    serdeable: new VariantSerdeable(
        [
            NodeSubscriptionFrame.serdeable,
            NodeNotificationFrame.serdeable,
            NodeSyncFrame.serdeable,
            NetworkSubscriptionFrame.serdeable,
            NetworkNotificationFrame.serdeable,
        ],
        (frame) => frame.frameType,
    ),
};
