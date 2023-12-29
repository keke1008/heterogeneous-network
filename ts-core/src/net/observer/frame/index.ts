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

import { VariantSerdeable } from "@core/serde";
import { NodeFrame, NodeNotificationFrame, NodeSubscriptionFrame } from "./node";
import { NetworkFrame, NetworkNotificationFrame, NetworkSubscriptionFrame } from "./network";

export type ObserverFrame = NodeFrame | NetworkFrame;

export const ObserverFrame = {
    serdeable: new VariantSerdeable(
        [
            NodeNotificationFrame.serdeable,
            NodeSubscriptionFrame.serdeable,
            NetworkNotificationFrame.serdeable,
            NetworkSubscriptionFrame.serdeable,
        ],
        (frame) => frame.frameType,
    ),
};
