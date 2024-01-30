import { ConstantSerdeable, ObjectSerdeable, TransformSerdeable, VariantSerdeable, VectorSerdeable } from "@core/serde";
import { FrameType } from "./common";
import { ClusterId, Cost, NoCluster, NodeId } from "@core/net/node";
import { LocalNotification } from "@core/net/notification";
import { match } from "ts-pattern";

enum NodeNotificationEntryType {
    SelfUpdated = 1,
    NeighborUpdated = 2,
    NeighborRemoved = 3,
    FrameReceived = 4,
}

export class SelfUpdatedEntry {
    readonly entryType = NodeNotificationEntryType.SelfUpdated as const;
    clusterId: ClusterId | NoCluster;
    cost: Cost;

    constructor(opts: { cost: Cost; clusterId: ClusterId | NoCluster }) {
        this.cost = opts.cost;
        this.clusterId = opts.clusterId ?? new NoCluster();
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ clusterId: ClusterId.serdeable, cost: Cost.serdeable }),
        (obj) => new SelfUpdatedEntry(obj),
        (frame) => frame,
    );
}

export class NeighborUpdatedEntry {
    readonly entryType = NodeNotificationEntryType.NeighborUpdated as const;
    neighbor: NodeId;
    linkCost: Cost;

    constructor(opts: { neighbor: NodeId; linkCost: Cost }) {
        this.neighbor = opts.neighbor;
        this.linkCost = opts.linkCost;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ neighbor: NodeId.serdeable, linkCost: Cost.serdeable }),
        (obj) => new NeighborUpdatedEntry(obj),
        (frame) => frame,
    );
}

export class NeighborRemovedEntry {
    readonly entryType = NodeNotificationEntryType.NeighborRemoved as const;
    neighborId: NodeId;

    constructor(opts: { neighborId: NodeId }) {
        this.neighborId = opts.neighborId;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ neighborId: NodeId.serdeable }),
        (obj) => new NeighborRemovedEntry(obj),
        (frame) => frame,
    );
}

export class FrameReceivedEntry {
    readonly entryType = NodeNotificationEntryType.FrameReceived as const;
    static readonly serdeable = new ConstantSerdeable(new FrameReceivedEntry());
}

export type NodeNotificationEntry = SelfUpdatedEntry | NeighborUpdatedEntry | NeighborRemovedEntry | FrameReceivedEntry;
export const NodeNotificationEntry = {
    serdeable: new VariantSerdeable(
        [
            SelfUpdatedEntry.serdeable,
            NeighborUpdatedEntry.serdeable,
            NeighborRemovedEntry.serdeable,
            FrameReceivedEntry.serdeable,
        ],
        (frame) => frame.entryType,
    ),

    fromLocalNotification: (notification: LocalNotification): NodeNotificationEntry => {
        return match(notification)
            .with({ type: "SelfUpdated" }, (n) => new SelfUpdatedEntry(n))
            .with({ type: "NeighborRemoved" }, (n) => new NeighborRemovedEntry({ neighborId: n.nodeId }))
            .with({ type: "NeighborUpdated" }, (n) => {
                return new NeighborUpdatedEntry({ neighbor: n.neighbor, linkCost: n.linkCost });
            })
            .with({ type: "FrameReceived" }, () => new FrameReceivedEntry())
            .exhaustive();
    },
};

export class NodeNotificationFrame {
    readonly frameType = FrameType.NodeNotification as const;

    entries: NodeNotificationEntry[];

    constructor(opts: { entries: NodeNotificationEntry[] }) {
        this.entries = opts.entries;
    }

    static fromLocalNotifications(notifications: LocalNotification[]): NodeNotificationFrame {
        return new NodeNotificationFrame({
            entries: notifications.map(NodeNotificationEntry.fromLocalNotification),
        });
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ entries: new VectorSerdeable(NodeNotificationEntry.serdeable) }),
        (obj) => new NodeNotificationFrame(obj),
        (frame) => frame,
    );
}

export class NodeSubscriptionFrame {
    readonly frameType = FrameType.NodeSubscription as const;

    static readonly serdeable = new ConstantSerdeable(new NodeSubscriptionFrame());
}

export type NodeFrame = NodeSubscriptionFrame | NodeNotificationEntry;
