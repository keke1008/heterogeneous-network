import { Cost, NodeId } from "@core/net/routing";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { FrameType, FRAME_TYPE_SERIALIXED_LENGTH, serializeFrameType } from "./common";
import { Err, Ok } from "oxide.ts";
import { NetNotification } from "@core/net/notification";
import { DeserializeResult, InvalidValueError } from "@core/serde";

export enum NotifyContent {
    NodeUpdated = 0x01,
    NodeRemoved = 0x02,
    LinkUpdated = 0x03,
    LinkRemoved = 0x04,
}

const serializeNotifyContent = (content: NotifyContent, writer: BufferWriter): void => {
    writer.writeByte(content);
};

export class InvalidNotifyContentError {
    constructor(public value: number) {}
}

const deserializeNotifyContent = (reader: BufferReader): DeserializeResult<NotifyContent> => {
    const content = reader.readByte();
    if (content in NotifyContent) {
        return Ok(content as NotifyContent);
    } else {
        return Err(new InvalidValueError(`Invalid notify content: ${content}`));
    }
};

const NOTIFY_CONTENT_SERIALIZED_LENGTH = 1;

export class NodeUpdatedFrame {
    readonly type = FrameType.Notify;
    readonly content = NotifyContent.NodeUpdated;
    readonly nodeId: NodeId;
    readonly cost: Cost;

    constructor(args: { nodeId: NodeId; cost: Cost }) {
        this.nodeId = args.nodeId;
        this.cost = args.cost;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NodeUpdatedFrame> {
        return NodeId.deserialize(reader).andThen((nodeId) => {
            return Cost.deserialize(reader).map((cost) => {
                return new NodeUpdatedFrame({ nodeId, cost });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.type, writer);
        serializeNotifyContent(this.content, writer);
        this.nodeId.serialize(writer);
        this.cost.serialize(writer);
    }

    serializedLength(): number {
        return (
            FRAME_TYPE_SERIALIXED_LENGTH +
            NOTIFY_CONTENT_SERIALIZED_LENGTH +
            this.nodeId.serializedLength() +
            this.cost.serializedLength()
        );
    }

    intoNotification(): NetNotification {
        return {
            type: "NodeUpdated",
            nodeId: this.nodeId,
            nodeCost: this.cost,
        };
    }
}

export class NodeRemovedFrame {
    readonly type = FrameType.Notify;
    readonly content = NotifyContent.NodeRemoved;
    readonly nodeId: NodeId;

    constructor(args: { nodeId: NodeId }) {
        this.nodeId = args.nodeId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NodeRemovedFrame> {
        return NodeId.deserialize(reader).map((nodeId) => {
            return new NodeRemovedFrame({ nodeId });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.type, writer);
        serializeNotifyContent(this.content, writer);
        this.nodeId.serialize(writer);
    }

    serializedLength(): number {
        return FRAME_TYPE_SERIALIXED_LENGTH + NOTIFY_CONTENT_SERIALIZED_LENGTH + this.nodeId.serializedLength();
    }

    intoNotification(): NetNotification {
        return {
            type: "NodeRemoved",
            nodeId: this.nodeId,
        };
    }
}

export class LinkUpdatedFrame {
    readonly type = FrameType.Notify;
    readonly content = NotifyContent.LinkUpdated;
    readonly nodeId1: NodeId;
    readonly nodeId2: NodeId;
    readonly cost: Cost;

    constructor(args: { nodeId1: NodeId; nodeId2: NodeId; cost: Cost }) {
        this.nodeId1 = args.nodeId1;
        this.nodeId2 = args.nodeId2;
        this.cost = args.cost;
    }

    static deserialize(reader: BufferReader): DeserializeResult<LinkUpdatedFrame> {
        return NodeId.deserialize(reader)
            .andThen((nodeId1) => {
                return NodeId.deserialize(reader).map((nodeId2) => {
                    return { nodeId1, nodeId2 };
                });
            })
            .andThen((nodeIds) => {
                return Cost.deserialize(reader).map((cost) => {
                    return new LinkUpdatedFrame({ ...nodeIds, cost });
                });
            });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.type, writer);
        serializeNotifyContent(this.content, writer);
        this.nodeId1.serialize(writer);
        this.nodeId2.serialize(writer);
        this.cost.serialize(writer);
    }

    serializedLength(): number {
        return (
            FRAME_TYPE_SERIALIXED_LENGTH +
            NOTIFY_CONTENT_SERIALIZED_LENGTH +
            this.nodeId1.serializedLength() +
            this.nodeId2.serializedLength() +
            this.cost.serializedLength()
        );
    }

    intoNotification(): NetNotification {
        return {
            type: "LinkUpdated",
            nodeId1: this.nodeId1,
            nodeId2: this.nodeId2,
            linkCost: this.cost,
        };
    }
}

export class LinkRemovedFrame {
    readonly type = FrameType.Notify;
    readonly content = NotifyContent.LinkRemoved;
    readonly nodeId1: NodeId;
    readonly nodeId2: NodeId;

    constructor(args: { nodeId1: NodeId; nodeId2: NodeId }) {
        this.nodeId1 = args.nodeId1;
        this.nodeId2 = args.nodeId2;
    }

    static deserialize(reader: BufferReader): DeserializeResult<LinkRemovedFrame> {
        return NodeId.deserialize(reader).andThen((nodeId1) => {
            return NodeId.deserialize(reader).map((nodeId2) => {
                return new LinkRemovedFrame({ nodeId1, nodeId2 });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.type, writer);
        serializeNotifyContent(this.content, writer);
        this.nodeId1.serialize(writer);
        this.nodeId2.serialize(writer);
    }

    serializedLength(): number {
        return (
            FRAME_TYPE_SERIALIXED_LENGTH +
            NOTIFY_CONTENT_SERIALIZED_LENGTH +
            this.nodeId1.serializedLength() +
            this.nodeId2.serializedLength()
        );
    }

    intoNotification(): NetNotification {
        return {
            type: "LinkRemoved",
            nodeId1: this.nodeId1,
            nodeId2: this.nodeId2,
        };
    }
}

export type NotifyFrame = NodeUpdatedFrame | NodeRemovedFrame | LinkUpdatedFrame | LinkRemovedFrame;

export const deserializeNotifyFrame = (reader: BufferReader): DeserializeResult<NotifyFrame> => {
    const content = deserializeNotifyContent(reader);
    if (content.isErr()) {
        return Err(content.unwrapErr());
    }

    switch (content.unwrap()) {
        case NotifyContent.NodeUpdated:
            return NodeUpdatedFrame.deserialize(reader);
        case NotifyContent.NodeRemoved:
            return NodeRemovedFrame.deserialize(reader);
        case NotifyContent.LinkUpdated:
            return LinkUpdatedFrame.deserialize(reader);
        case NotifyContent.LinkRemoved:
            return LinkRemovedFrame.deserialize(reader);
    }
};

export const fromNotification = (notification: NetNotification): NotifyFrame => {
    switch (notification.type) {
        case "NodeUpdated":
            return new NodeUpdatedFrame({ nodeId: notification.nodeId, cost: notification.nodeCost });
        case "NodeRemoved":
            return new NodeRemovedFrame({ nodeId: notification.nodeId });
        case "LinkUpdated":
            return new LinkUpdatedFrame({
                nodeId1: notification.nodeId1,
                nodeId2: notification.nodeId2,
                cost: notification.linkCost,
            });
        case "LinkRemoved":
            return new LinkRemovedFrame({ nodeId1: notification.nodeId1, nodeId2: notification.nodeId2 });
    }
};
