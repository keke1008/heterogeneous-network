import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Cost, NetworkUpdate, NodeId } from "@core/net/node";
import { DeserializeResult, DeserializeVector, InvalidValueError, SerializeVector } from "@core/serde";
import { FRAME_TYPE_SERIALIZED_LENGTH, FrameType, serializeFrameType } from "./common";
import { Err, Ok } from "oxide.ts";
import { match } from "ts-pattern";

enum NetworkNotificationEntryType {
    NodeUpdated = 1,
    NodeRemoved = 2,
    LinkUpdated = 3,
    LinkRemoved = 4,
}

const NETWORK_NOTIFICATION_ENTRY_TYPE_SERIALIZED_LENGTH = 1;

const serializeNetworkNotificationEntryType = (type: NetworkNotificationEntryType, writer: BufferWriter): void => {
    writer.writeByte(type);
};

export class NetworkNodeUpdatedNotificationEntry {
    readonly entryType = NetworkNotificationEntryType.NodeUpdated as const;
    node: NodeId;
    cost: Cost;

    constructor(args: { node: NodeId; cost: Cost }) {
        this.node = args.node;
        this.cost = args.cost;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NetworkNodeUpdatedNotificationEntry> {
        return NodeId.deserialize(reader).andThen((node) => {
            return Cost.deserialize(reader).map((cost) => {
                return new NetworkNodeUpdatedNotificationEntry({ node, cost });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeNetworkNotificationEntryType(this.entryType, writer);
        this.node.serialize(writer);
        this.cost.serialize(writer);
    }

    serializedLength(): number {
        return (
            NETWORK_NOTIFICATION_ENTRY_TYPE_SERIALIZED_LENGTH +
            this.node.serializedLength() +
            this.cost.serializedLength()
        );
    }
}

export class NetworkNodeRemovedNotificationEntry {
    readonly entryType = NetworkNotificationEntryType.NodeRemoved as const;
    node: NodeId;

    constructor(args: { node: NodeId }) {
        this.node = args.node;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NetworkNodeRemovedNotificationEntry> {
        return NodeId.deserialize(reader).map((node) => {
            return new NetworkNodeRemovedNotificationEntry({ node });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeNetworkNotificationEntryType(this.entryType, writer);
        this.node.serialize(writer);
    }

    serializedLength(): number {
        return NETWORK_NOTIFICATION_ENTRY_TYPE_SERIALIZED_LENGTH + this.node.serializedLength();
    }
}

export class NetworkLinkUpdatedNotificationEntry {
    readonly entryType = NetworkNotificationEntryType.LinkUpdated as const;
    node1: NodeId;
    node2: NodeId;
    cost: Cost;

    constructor(args: { node1: NodeId; node2: NodeId; cost: Cost }) {
        this.node1 = args.node1;
        this.node2 = args.node2;
        this.cost = args.cost;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NetworkLinkUpdatedNotificationEntry> {
        return NodeId.deserialize(reader).andThen((node1) => {
            return NodeId.deserialize(reader).andThen((node2) => {
                return Cost.deserialize(reader).map((cost) => {
                    return new NetworkLinkUpdatedNotificationEntry({ node1, node2, cost });
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeNetworkNotificationEntryType(this.entryType, writer);
        this.node1.serialize(writer);
        this.node2.serialize(writer);
        this.cost.serialize(writer);
    }

    serializedLength(): number {
        return (
            NETWORK_NOTIFICATION_ENTRY_TYPE_SERIALIZED_LENGTH +
            this.node1.serializedLength() +
            this.node2.serializedLength() +
            this.cost.serializedLength()
        );
    }
}

export class NetworkLinkRemovedNotificationEntry {
    readonly entryType = NetworkNotificationEntryType.LinkRemoved as const;
    node1: NodeId;
    node2: NodeId;

    constructor(args: { node1: NodeId; node2: NodeId }) {
        this.node1 = args.node1;
        this.node2 = args.node2;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NetworkLinkRemovedNotificationEntry> {
        return NodeId.deserialize(reader).andThen((node1) => {
            return NodeId.deserialize(reader).map((node2) => {
                return new NetworkLinkRemovedNotificationEntry({ node1, node2 });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeNetworkNotificationEntryType(this.entryType, writer);
        this.node1.serialize(writer);
        this.node2.serialize(writer);
    }

    serializedLength(): number {
        return (
            NETWORK_NOTIFICATION_ENTRY_TYPE_SERIALIZED_LENGTH +
            this.node1.serializedLength() +
            this.node2.serializedLength()
        );
    }
}

export type NetworkNotificationEntry =
    | NetworkNodeUpdatedNotificationEntry
    | NetworkNodeRemovedNotificationEntry
    | NetworkLinkUpdatedNotificationEntry
    | NetworkLinkRemovedNotificationEntry;

type NetworkNotificationEntryClass =
    | typeof NetworkNodeUpdatedNotificationEntry
    | typeof NetworkNodeRemovedNotificationEntry
    | typeof NetworkLinkUpdatedNotificationEntry
    | typeof NetworkLinkRemovedNotificationEntry;

const networkNotificationEntryDeserializer = {
    deserialize(reader: BufferReader): DeserializeResult<NetworkNotificationEntry> {
        const entryType = reader.readByte();
        const map: Record<NetworkNotificationEntryType, NetworkNotificationEntryClass> = {
            [NetworkNotificationEntryType.NodeUpdated]: NetworkNodeUpdatedNotificationEntry,
            [NetworkNotificationEntryType.NodeRemoved]: NetworkNodeRemovedNotificationEntry,
            [NetworkNotificationEntryType.LinkUpdated]: NetworkLinkUpdatedNotificationEntry,
            [NetworkNotificationEntryType.LinkRemoved]: NetworkLinkRemovedNotificationEntry,
        };

        return entryType in map
            ? map[entryType as NetworkNotificationEntryType].deserialize(reader)
            : Err(new InvalidValueError(`Invalid network notification entry type: ${entryType}`));
    },
};

export class NetworkNotificationFrame {
    readonly frameType = FrameType.NetworkNotification as const;
    entries: NetworkNotificationEntry[];

    constructor(entries: NetworkNotificationEntry[]) {
        this.entries = entries;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NetworkNotificationFrame> {
        return new DeserializeVector(networkNotificationEntryDeserializer).deserialize(reader).map((entries) => {
            return new NetworkNotificationFrame(entries);
        });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.frameType, writer);
        return new SerializeVector(this.entries).serialize(writer);
    }

    serializedLength(): number {
        return FRAME_TYPE_SERIALIZED_LENGTH + new SerializeVector(this.entries).serializedLength();
    }
}

const deserializeNetworkNotificationFrame = NetworkNotificationFrame.deserialize;

export class NetworkSubscriptionFrame {
    readonly frameType = FrameType.NetworkSubscription as const;

    static deserialize(): DeserializeResult<NetworkSubscriptionFrame> {
        return Ok(new NetworkSubscriptionFrame());
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.frameType, writer);
    }

    serializedLength(): number {
        return FRAME_TYPE_SERIALIZED_LENGTH;
    }
}

const deserializeNetworkSubscriptionFrame = NetworkSubscriptionFrame.deserialize;

export type NetworkFrame = NetworkNotificationFrame | NetworkSubscriptionFrame;

export const deserializeNetworkFrame = (
    frameType: FrameType.NetworkNotification | FrameType.NetworkSubscription,
    reader: BufferReader,
): DeserializeResult<NetworkFrame> => {
    switch (frameType) {
        case FrameType.NetworkNotification:
            return deserializeNetworkNotificationFrame(reader);
        case FrameType.NetworkSubscription:
            return deserializeNetworkSubscriptionFrame();
    }
};

export const createNetworkNotificationEntryFromNetworkUpdate = (update: NetworkUpdate): NetworkNotificationEntry => {
    return match(update)
        .with({ type: "NodeUpdated" }, (update) => {
            return new NetworkNodeUpdatedNotificationEntry({
                node: update.id,
                cost: update.cost,
            });
        })
        .with({ type: "NodeRemoved" }, (update) => {
            return new NetworkNodeRemovedNotificationEntry({ node: update.id });
        })
        .with({ type: "LinkUpdated" }, (update) => {
            return new NetworkLinkUpdatedNotificationEntry({
                node1: update.source,
                node2: update.destination,
                cost: update.cost,
            });
        })
        .with({ type: "LinkRemoved" }, (update) => {
            return new NetworkLinkRemovedNotificationEntry({
                node1: update.source,
                node2: update.destination,
            });
        })
        .exhaustive();
};
