import { ConstantSerdeable, ObjectSerdeable, TransformSerdeable, VariantSerdeable } from "@core/serde";
import { FrameType } from "./common";
import { ClusterId, Cost, NoCluster, NodeId, Source } from "@core/net/node";
import { LocalNotification } from "@core/net/notification";
import { match } from "ts-pattern";

enum NodeNotificationType {
    SelfUpdated = 1,
    NeighborUpdated = 2,
    NeighborRemoved = 3,
    FrameReceived = 4,
}

export class SelfUpdatedFrame {
    readonly frameType = FrameType.NodeNotification as const;
    readonly notificationType = NodeNotificationType.SelfUpdated as const;
    clusterId: ClusterId | NoCluster;
    cost: Cost;

    constructor(opts: { cost: Cost; clusterId: ClusterId | NoCluster }) {
        this.cost = opts.cost;
        this.clusterId = opts.clusterId ?? new NoCluster();
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ clusterId: ClusterId.serdeable, cost: Cost.serdeable }),
        (obj) => new SelfUpdatedFrame(obj),
        (frame) => frame,
    );
}

export class NeighborUpdatedFrame {
    readonly frameType = FrameType.NodeNotification as const;
    readonly notificationType = NodeNotificationType.NeighborUpdated as const;
    localClusterId: ClusterId | NoCluster;
    localCost: Cost;
    neighbor: Source;
    neighborCost: Cost;
    linkCost: Cost;

    constructor(opts: {
        localClusterId: ClusterId | NoCluster;
        localCost: Cost;
        neighbor: Source;
        neighborCost: Cost;
        linkCost: Cost;
    }) {
        this.localClusterId = opts.localClusterId;
        this.localCost = opts.localCost;
        this.neighbor = opts.neighbor;
        this.neighborCost = opts.neighborCost;
        this.linkCost = opts.linkCost;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            localClusterId: ClusterId.serdeable,
            localCost: Cost.serdeable,
            neighbor: Source.serdeable,
            neighborCost: Cost.serdeable,
            linkCost: Cost.serdeable,
        }),
        (obj) => new NeighborUpdatedFrame(obj),
        (frame) => frame,
    );
}

export class NeighborRemovedFrame {
    readonly frameType = FrameType.NodeNotification as const;
    readonly notificationType = NodeNotificationType.NeighborRemoved as const;
    neighborId: NodeId;

    constructor(opts: { neighborId: NodeId }) {
        this.neighborId = opts.neighborId;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ neighborId: NodeId.serdeable }),
        (obj) => new NeighborRemovedFrame(obj),
        (frame) => frame,
    );
}

export class FrameReceivedFrame {
    readonly frameType = FrameType.NodeNotification as const;
    readonly notificationType = NodeNotificationType.FrameReceived as const;

    static readonly serdeable = new ConstantSerdeable(new FrameReceivedFrame());
}

export type NodeNotificationFrame = SelfUpdatedFrame | NeighborUpdatedFrame | NeighborRemovedFrame | FrameReceivedFrame;

export const NodeNotificationFrame = {
    serdeable: new VariantSerdeable(
        [
            SelfUpdatedFrame.serdeable,
            NeighborUpdatedFrame.serdeable,
            NeighborRemovedFrame.serdeable,
            FrameReceivedFrame.serdeable,
        ],
        (frame) => frame.notificationType,
    ),
};

export class NodeSubscriptionFrame {
    readonly frameType = FrameType.NodeSubscription as const;

    static readonly serdeable = new ConstantSerdeable(new NodeSubscriptionFrame());
}

export const createNodeNotificationFrameFromLocalNotification = (
    notification: LocalNotification,
    localClusterId: ClusterId | NoCluster,
    localCost: Cost,
): NodeNotificationFrame => {
    return match(notification)
        .with({ type: "SelfUpdated" }, (n) => new SelfUpdatedFrame(n))
        .with({ type: "NeighborRemoved" }, (n) => new NeighborRemovedFrame({ neighborId: n.nodeId }))
        .with({ type: "NeighborUpdated" }, (n) => {
            return new NeighborUpdatedFrame({
                localClusterId: localClusterId,
                localCost,
                neighbor: n.neighbor,
                neighborCost: n.neighborCost,
                linkCost: n.linkCost,
            });
        })
        .with({ type: "FrameReceived" }, () => new FrameReceivedFrame())
        .exhaustive();
};

export type NodeFrame = NodeSubscriptionFrame | NodeNotificationFrame;
