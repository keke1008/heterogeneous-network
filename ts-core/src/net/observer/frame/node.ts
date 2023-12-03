import { BufferReader, BufferWriter } from "@core/net/buffer";
import { DeserializeResult, InvalidValueError } from "@core/serde";
import { Err, Ok } from "oxide.ts";
import { FRAME_TYPE_SERIALIZED_LENGTH, FrameType, serializeFrameType } from "./common";
import { Cost, NodeId } from "@core/net/node";
import { LocalNotification } from "@core/net/notification";

enum NodeNotificationType {
    SelfUpdated = 1,
    NeighborUpdated = 2,
    NeighborRemoved = 3,
}

const NODE_NOTIFICATION_TYPE_SERIALIZED_LENGTH = 1;

const serializeNodeNotificationType = (type: NodeNotificationType, writer: BufferWriter): void => {
    writer.writeByte(type);
};

export class SelfUpdatedFrame {
    readonly frameType = FrameType.NodeNotification as const;
    readonly notificationType = NodeNotificationType.SelfUpdated as const;
    cost: Cost;

    constructor(opts: { cost: Cost }) {
        this.cost = opts.cost;
    }

    static deserialize(reader: BufferReader): DeserializeResult<SelfUpdatedFrame> {
        return Cost.deserialize(reader).map((cost) => {
            return new SelfUpdatedFrame({ cost });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.frameType, writer);
        serializeNodeNotificationType(this.notificationType, writer);
        this.cost.serialize(writer);
    }

    serializedLength(): number {
        return FRAME_TYPE_SERIALIZED_LENGTH + NODE_NOTIFICATION_TYPE_SERIALIZED_LENGTH + this.cost.serializedLength();
    }
}

export class NeighborUpdatedFrame {
    readonly frameType = FrameType.NodeNotification as const;
    readonly notificationType = NodeNotificationType.NeighborUpdated as const;
    sourceCost: Cost;
    neighborId: NodeId;
    linkCost: Cost;
    neighborCost: Cost;

    constructor(opts: { sourceCost: Cost; neighborId: NodeId; linkCost: Cost; neighborCost: Cost }) {
        this.sourceCost = opts.sourceCost;
        this.neighborId = opts.neighborId;
        this.linkCost = opts.linkCost;
        this.neighborCost = opts.neighborCost;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NeighborUpdatedFrame> {
        return Cost.deserialize(reader).andThen((sourceCost) => {
            return NodeId.deserialize(reader).andThen((neighborId) => {
                return Cost.deserialize(reader).andThen((linkCost) => {
                    return Cost.deserialize(reader).map((neighborCost) => {
                        return new NeighborUpdatedFrame({ sourceCost, neighborId, linkCost, neighborCost });
                    });
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.frameType, writer);
        serializeNodeNotificationType(this.notificationType, writer);
        this.neighborId.serialize(writer);
        this.linkCost.serialize(writer);
        this.neighborCost.serialize(writer);
    }

    serializedLength(): number {
        return (
            FRAME_TYPE_SERIALIZED_LENGTH +
            NODE_NOTIFICATION_TYPE_SERIALIZED_LENGTH +
            this.neighborId.serializedLength() +
            this.linkCost.serializedLength() +
            this.neighborCost.serializedLength()
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

type NodeNotificationFrameClass = typeof SelfUpdatedFrame | typeof NeighborUpdatedFrame | typeof NeighborRemovedFrame;

const deserializeNodeNotificationType = (reader: BufferReader): DeserializeResult<NodeNotificationFrameClass> => {
    const type = reader.readByte();
    const map: Record<NodeNotificationType, NodeNotificationFrameClass> = {
        [NodeNotificationType.SelfUpdated]: SelfUpdatedFrame,
        [NodeNotificationType.NeighborUpdated]: NeighborUpdatedFrame,
        [NodeNotificationType.NeighborRemoved]: NeighborRemovedFrame,
    };

    return type in map
        ? Ok(map[type as NodeNotificationType])
        : Err(new InvalidValueError(`Invalid node notification type: ${type}`));
};

export type NodeNotificationFrame = SelfUpdatedFrame | NeighborUpdatedFrame | NeighborRemovedFrame;

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
): NodeNotificationFrame => {
    switch (notification.type) {
        case "SelfUpdated":
            return new SelfUpdatedFrame({ cost: notification.cost });
        case "NeighborUpdated":
            return new NeighborUpdatedFrame({
                sourceCost: notification.localCost,
                neighborId: notification.neighborId,
                neighborCost: notification.neighborCost,
                linkCost: notification.linkCost,
            });
        case "NeighborRemoved":
            return new NeighborRemovedFrame({ neighborId: notification.nodeId });
    }
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
