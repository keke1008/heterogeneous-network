import { BufferReader, BufferWriter } from "@core/net/buffer";
import { DeserializeResult, InvalidValueError } from "@core/serde";
import { Err, Ok } from "oxide.ts";
import { FRAME_TYPE_SERIALIZED_LENGTH, FrameType, serializeFrameType } from "./common";
import { Cost, NodeId } from "@core/net/node";
import { LocalNotification } from "@core/net/notification";
import { match } from "ts-pattern";

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
    neighborCost: Cost;
    linkCost: Cost;

    constructor(opts: { localCost: Cost; neighborId: NodeId; neighborCost: Cost; linkCost: Cost }) {
        this.sourceCost = opts.localCost;
        this.neighborId = opts.neighborId;
        this.neighborCost = opts.neighborCost;
        this.linkCost = opts.linkCost;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NeighborUpdatedFrame> {
        return Cost.deserialize(reader).andThen((localCost) => {
            return NodeId.deserialize(reader).andThen((neighborId) => {
                return Cost.deserialize(reader).andThen((neighborCost) => {
                    return Cost.deserialize(reader).map((linkCost) => {
                        return new NeighborUpdatedFrame({ localCost, neighborId, neighborCost, linkCost });
                    });
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.frameType, writer);
        serializeNodeNotificationType(this.notificationType, writer);
        this.neighborId.serialize(writer);
        this.neighborCost.serialize(writer);
        this.linkCost.serialize(writer);
    }

    serializedLength(): number {
        return (
            FRAME_TYPE_SERIALIZED_LENGTH +
            NODE_NOTIFICATION_TYPE_SERIALIZED_LENGTH +
            this.neighborId.serializedLength() +
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
    return match(notification)
        .with({ type: "SelfUpdated" }, (n) => new SelfUpdatedFrame({ cost: n.cost }))
        .with({ type: "NeighborRemoved" }, (n) => new NeighborRemovedFrame({ neighborId: n.nodeId }))
        .with({ type: "NeighborUpdated" }, (n) => {
            return new NeighborUpdatedFrame({
                localCost: n.localCost,
                neighborId: n.neighborId,
                neighborCost: n.neighborCost,
                linkCost: n.linkCost,
            });
        })
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
