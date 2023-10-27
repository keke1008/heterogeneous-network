import { BufferReader, BufferWriter } from "../buffer";
import { FrameId } from "../link";
import { NodeId } from "../routing";

export enum FrameType {
    Subscription = 0x01,
    Notification = 0x02,
}

export class SubscriptionFrame {
    readonly type = FrameType.Subscription as const;
    readonly frameId: FrameId;

    constructor(opts: { frameId: FrameId }) {
        this.frameId = opts.frameId;
    }

    static deserialize(reader: BufferReader): SubscriptionFrame {
        const frameId = FrameId.deserialize(reader);
        return new SubscriptionFrame({ frameId });
    }

    serialize(writer: BufferWriter): void {
        this.frameId.serialize(writer);
    }

    serializedLength(): number {
        return 1 + this.frameId.serializedLength();
    }
}

export enum NotificationTarget {
    Node = 0x01,
    Link = 0x02,
}

export enum NotificationAction {
    Added = 0x01,
    Removed = 0x02,
    Updated = 0x03,
}

export class NodeNotificationFrame {
    readonly type = FrameType.Notification as const;
    readonly target = NotificationTarget.Node as const;
    readonly action: NotificationAction;
    readonly nodeIds: NodeId[];

    constructor(opts: { action: NotificationAction; nodeIds: NodeId[] }) {
        this.action = opts.action;
        this.nodeIds = opts.nodeIds;
    }

    static deserialize(reader: BufferReader): NodeNotificationFrame {
        const action = reader.readByte() as NotificationAction;
        const nodeIds: NodeId[] = [];
        while (reader.readableLength() > 0) {
            nodeIds.push(NodeId.deserialize(reader));
        }

        return new NodeNotificationFrame({ action, nodeIds });
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.type);
        writer.writeByte(this.target);
        writer.writeByte(this.action);
        for (const nodeId of this.nodeIds) {
            nodeId.serialize(writer);
        }
    }

    serializedLength(): number {
        return 1 + 1 + this.nodeIds.reduce((acc, nodeId) => acc + nodeId.serializedLength(), 0);
    }
}

export class LinkNotificationFrame {
    readonly type = FrameType.Notification as const;
    readonly target = NotificationTarget.Link as const;
    readonly action: NotificationAction;
    readonly nodeId: NodeId;
    readonly linkIds: NodeId[];

    constructor(opts: { action: NotificationAction; nodeId: NodeId; linkIds: NodeId[] }) {
        this.action = opts.action;
        this.nodeId = opts.nodeId;
        this.linkIds = opts.linkIds;
    }

    static deserialize(reader: BufferReader): LinkNotificationFrame {
        const action = reader.readByte() as NotificationAction;
        const nodeId = NodeId.deserialize(reader);
        const linkIds: NodeId[] = [];
        while (reader.readableLength() > 0) {
            linkIds.push(NodeId.deserialize(reader));
        }

        return new LinkNotificationFrame({ action, nodeId, linkIds });
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.type);
        writer.writeByte(this.target);
        writer.writeByte(this.action);
        this.nodeId.serialize(writer);
        for (const linkId of this.linkIds) {
            linkId.serialize(writer);
        }
    }

    serializedLength(): number {
        return (
            1 +
            1 +
            this.nodeId.serializedLength() +
            this.linkIds.reduce((acc, linkId) => acc + linkId.serializedLength(), 0)
        );
    }
}

export type NotificationFrame = NodeNotificationFrame | LinkNotificationFrame;

export const deserializeFrame = (reader: BufferReader): SubscriptionFrame | NotificationFrame => {
    const type = reader.readByte() as FrameType;
    if (type === FrameType.Subscription) {
        return SubscriptionFrame.deserialize(reader);
    }

    const target = reader.readByte() as NotificationTarget;
    switch (target) {
        case NotificationTarget.Node:
            return NodeNotificationFrame.deserialize(reader);
        case NotificationTarget.Link:
            return LinkNotificationFrame.deserialize(reader);
        default:
            throw new Error(`Unknown notification target ${target}`);
    }
};

export const serializeFrame = (frame: SubscriptionFrame | NotificationFrame): BufferReader => {
    const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
    frame.serialize(writer);
    return new BufferReader(writer.unwrapBuffer());
};

export type Notification =
    | { target: "node"; action: "added"; nodeIds: NodeId[] }
    | { target: "node"; action: "removed"; nodeIds: NodeId[] }
    | { target: "link"; action: "added"; nodeId: NodeId; linkIds: NodeId[] }
    | { target: "link"; action: "removed"; nodeId: NodeId; linkIds: NodeId[] };

export const notificationToFrame = (notification: Notification): NotificationFrame => {
    const action = notification.action === "added" ? NotificationAction.Added : NotificationAction.Removed;
    if (notification.target === "node") {
        return new NodeNotificationFrame({
            action,
            nodeIds: notification.nodeIds,
        });
    } else {
        return new LinkNotificationFrame({
            action,
            nodeId: notification.nodeId,
            linkIds: notification.linkIds,
        });
    }
};

export const frameToNotification = (frame: NotificationFrame): Notification => {
    if (frame.target === NotificationTarget.Node) {
        return {
            target: "node",
            action: frame.action === NotificationAction.Added ? "added" : "removed",
            nodeIds: frame.nodeIds,
        };
    } else {
        return {
            target: "link",
            action: frame.action === NotificationAction.Added ? "added" : "removed",
            nodeId: frame.nodeId,
            linkIds: frame.linkIds,
        };
    }
};
