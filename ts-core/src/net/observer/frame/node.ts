import { BufferReader, BufferWriter } from "@core/net/buffer";
import { DeserializeResult, InvalidValueError } from "@core/serde";
import { Err, Ok } from "oxide.ts";
import { FRAME_TYPE_SERIALIZED_LENGTH, FrameType, serializeFrameType } from "./common";
import { ClusterId, Cost, NoCluster, NodeId, Source } from "@core/net/node";
import { LocalNotification } from "@core/net/notification";
import { match } from "ts-pattern";

enum NodeNotificationType {
    SelfUpdated = 1,
    NeighborUpdated = 2,
    NeighborRemoved = 3,
    FrameReceived = 4,
}

const NODE_NOTIFICATION_TYPE_SERIALIZED_LENGTH = 1;

const serializeNodeNotificationType = (type: NodeNotificationType, writer: BufferWriter): void => {
    writer.writeByte(type);
};

export class SelfUpdatedFrame {
    readonly frameType = FrameType.NodeNotification as const;
    readonly notificationType = NodeNotificationType.SelfUpdated as const;
    clusterId: ClusterId | NoCluster;
    cost: Cost;

    constructor(opts: { cost: Cost; clusterId: ClusterId | NoCluster }) {
        this.cost = opts.cost;
        this.clusterId = opts.clusterId ?? new NoCluster();
    }

    static deserialize(reader: BufferReader): DeserializeResult<SelfUpdatedFrame> {
        return ClusterId.deserialize(reader).andThen((clusterId) => {
            return Cost.deserialize(reader).map((cost) => {
                return new SelfUpdatedFrame({ clusterId, cost });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.frameType, writer);
        serializeNodeNotificationType(this.notificationType, writer);
        this.clusterId.serialize(writer);
        this.cost.serialize(writer);
    }

    serializedLength(): number {
        return (
            FRAME_TYPE_SERIALIZED_LENGTH +
            NODE_NOTIFICATION_TYPE_SERIALIZED_LENGTH +
            this.clusterId.serializedLength() +
            this.cost.serializedLength()
        );
    }
}

export class NeighborUpdatedFrame {
    readonly frameType = FrameType.NodeNotification as const;
    readonly notificationType = NodeNotificationType.NeighborUpdated as const;
    localCusterId: ClusterId | NoCluster;
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
        this.localCusterId = opts.localClusterId;
        this.localCost = opts.localCost;
        this.neighbor = opts.neighbor;
        this.neighborCost = opts.neighborCost;
        this.linkCost = opts.linkCost;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NeighborUpdatedFrame> {
        return ClusterId.deserialize(reader).andThen((localClusterId) => {
            return Cost.deserialize(reader).andThen((localCost) => {
                return Source.deserialize(reader).andThen((neighbor) => {
                    return Cost.deserialize(reader).andThen((neighborCost) => {
                        return Cost.deserialize(reader).map((linkCost) => {
                            return new NeighborUpdatedFrame({
                                localClusterId,
                                localCost,
                                neighbor,
                                neighborCost,
                                linkCost,
                            });
                        });
                    });
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.frameType, writer);
        serializeNodeNotificationType(this.notificationType, writer);
        this.localCusterId.serialize(writer);
        this.localCost.serialize(writer);
        this.neighbor.serialize(writer);
        this.neighborCost.serialize(writer);
        this.linkCost.serialize(writer);
    }

    serializedLength(): number {
        return (
            FRAME_TYPE_SERIALIZED_LENGTH +
            NODE_NOTIFICATION_TYPE_SERIALIZED_LENGTH +
            this.localCusterId.serializedLength() +
            this.localCost.serializedLength() +
            this.neighbor.serializedLength() +
            this.neighborCost.serializedLength() +
            this.linkCost.serializedLength()
        );
    }
}

export class NeighborRemovedFrame {
    readonly frameType = FrameType.NodeNotification as const;
    readonly notificationType = NodeNotificationType.NeighborRemoved as const;
    neighborId: NodeId;

    constructor(opts: { neighborId: NodeId }) {
        this.neighborId = opts.neighborId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NeighborRemovedFrame> {
        return NodeId.deserialize(reader).map((neighborId) => {
            return new NeighborRemovedFrame({ neighborId });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.frameType, writer);
        serializeNodeNotificationType(this.notificationType, writer);
        this.neighborId.serialize(writer);
    }

    serializedLength(): number {
        return (
            FRAME_TYPE_SERIALIZED_LENGTH + NODE_NOTIFICATION_TYPE_SERIALIZED_LENGTH + this.neighborId.serializedLength()
        );
    }
}

export class FrameReceivedFrame {
    readonly frameType = FrameType.NodeNotification as const;
    readonly notificationType = NodeNotificationType.FrameReceived as const;

    static deserialize(): DeserializeResult<FrameReceivedFrame> {
        return Ok(new FrameReceivedFrame());
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.frameType, writer);
        serializeNodeNotificationType(this.notificationType, writer);
    }

    serializedLength(): number {
        return FRAME_TYPE_SERIALIZED_LENGTH + NODE_NOTIFICATION_TYPE_SERIALIZED_LENGTH;
    }
}

type NodeNotificationFrameClass =
    | typeof SelfUpdatedFrame
    | typeof NeighborUpdatedFrame
    | typeof NeighborRemovedFrame
    | typeof FrameReceivedFrame;

const deserializeNodeNotificationType = (reader: BufferReader): DeserializeResult<NodeNotificationFrameClass> => {
    const type = reader.readByte();
    const map: Record<NodeNotificationType, NodeNotificationFrameClass> = {
        [NodeNotificationType.SelfUpdated]: SelfUpdatedFrame,
        [NodeNotificationType.NeighborUpdated]: NeighborUpdatedFrame,
        [NodeNotificationType.NeighborRemoved]: NeighborRemovedFrame,
        [NodeNotificationType.FrameReceived]: FrameReceivedFrame,
    };

    return type in map
        ? Ok(map[type as NodeNotificationType])
        : Err(new InvalidValueError(`Invalid node notification type: ${type}`));
};

export type NodeNotificationFrame = SelfUpdatedFrame | NeighborUpdatedFrame | NeighborRemovedFrame | FrameReceivedFrame;

const deserializeNodeNotificationFrame = (reader: BufferReader): DeserializeResult<NodeNotificationFrame> => {
    return deserializeNodeNotificationType(reader).andThen<NodeNotificationFrame>((cls) => {
        return cls.deserialize(reader);
    });
};

export class NodeSubscriptionFrame {
    readonly frameType = FrameType.NodeSubscription as const;

    static deserialize(): DeserializeResult<NodeSubscriptionFrame> {
        return Ok(new NodeSubscriptionFrame());
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.frameType, writer);
    }

    serializedLength(): number {
        return FRAME_TYPE_SERIALIZED_LENGTH;
    }
}

const deserializeNodeSubscriptionFrame = NodeSubscriptionFrame.deserialize;

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

export const deserializeNodeFrame = (
    frameType: FrameType.NodeSubscription | FrameType.NodeNotification,
    reader: BufferReader,
): DeserializeResult<NodeFrame> => {
    switch (frameType) {
        case FrameType.NodeSubscription:
            return deserializeNodeSubscriptionFrame();
        case FrameType.NodeNotification:
            return deserializeNodeNotificationFrame(reader);
    }
};
