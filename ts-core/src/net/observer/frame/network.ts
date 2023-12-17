import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Cost, NetworkUpdate, NodeId, Source } from "@core/net/node";
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
    node: Source;
    cost: Cost;

    constructor(args: { node: Source; cost: Cost }) {
        this.node = args.node;
        this.cost = args.cost;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NetworkNodeUpdatedNotificationEntry> {
        return Source.deserialize(reader).andThen((node) => {
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

    toNetworkUpdate(): NetworkUpdate {
        return { type: "NodeUpdated", node: this.node, cost: this.cost };
    }
}

export class NetworkNodeRemovedNotificationEntry {
    readonly entryType = NetworkNotificationEntryType.NodeRemoved as const;
    id: NodeId;

    constructor(args: { id: NodeId }) {
        this.id = args.id;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NetworkNodeRemovedNotificationEntry> {
        return NodeId.deserialize(reader).map((node) => {
            return new NetworkNodeRemovedNotificationEntry({ id: node });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeNetworkNotificationEntryType(this.entryType, writer);
        this.id.serialize(writer);
    }

    serializedLength(): number {
        return NETWORK_NOTIFICATION_ENTRY_TYPE_SERIALIZED_LENGTH + this.id.serializedLength();
    }

    toNetworkUpdate(): NetworkUpdate {
        return { type: "NodeRemoved", id: this.id };
    }
}

export class NetworkLinkUpdatedNotificationEntry {
    readonly entryType = NetworkNotificationEntryType.LinkUpdated as const;
    source: Source;
    sourceCost: Cost;
    destination: Source;
    destinationCost: Cost;
    linkCost: Cost;

    constructor(args: {
        source: Source;
        sourceCost: Cost;
        destination: Source;
        destinationCost: Cost;
        linkCost: Cost;
    }) {
        this.source = args.source;
        this.sourceCost = args.sourceCost;
        this.destination = args.destination;
        this.destinationCost = args.destinationCost;
        this.linkCost = args.linkCost;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NetworkLinkUpdatedNotificationEntry> {
        return Source.deserialize(reader).andThen((source) => {
            return Cost.deserialize(reader).andThen((sourceCost) => {
                return Source.deserialize(reader).andThen((destination) => {
                    return Cost.deserialize(reader).andThen((destinationCost) => {
                        return Cost.deserialize(reader).map((linkCost) => {
                            return new NetworkLinkUpdatedNotificationEntry({
                                source,
                                sourceCost,
                                destination,
                                destinationCost,
                                linkCost,
                            });
                        });
                    });
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeNetworkNotificationEntryType(this.entryType, writer);
        this.source.serialize(writer);
        this.sourceCost.serialize(writer);
        this.destination.serialize(writer);
        this.destinationCost.serialize(writer);
        this.linkCost.serialize(writer);
    }

    serializedLength(): number {
        return (
            NETWORK_NOTIFICATION_ENTRY_TYPE_SERIALIZED_LENGTH +
            this.source.serializedLength() +
            this.sourceCost.serializedLength() +
            this.destination.serializedLength() +
            this.destinationCost.serializedLength() +
            this.linkCost.serializedLength()
        );
    }

    toNetworkUpdate(): NetworkUpdate {
        return {
            type: "LinkUpdated",
            source: this.source,
            sourceCost: this.sourceCost,
            destination: this.destination,
            destinationCost: this.destinationCost,
            linkCost: this.linkCost,
        };
    }
}

export class NetworkLinkRemovedNotificationEntry {
    readonly entryType = NetworkNotificationEntryType.LinkRemoved as const;
    sourceId: NodeId;
    destinationId: NodeId;

    constructor(args: { sourceId: NodeId; destinationId: NodeId }) {
        this.sourceId = args.sourceId;
        this.destinationId = args.destinationId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NetworkLinkRemovedNotificationEntry> {
        return NodeId.deserialize(reader).andThen((sourceId) => {
            return NodeId.deserialize(reader).map((destinationId) => {
                return new NetworkLinkRemovedNotificationEntry({ sourceId, destinationId });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeNetworkNotificationEntryType(this.entryType, writer);
        this.sourceId.serialize(writer);
        this.destinationId.serialize(writer);
    }

    serializedLength(): number {
        return (
            NETWORK_NOTIFICATION_ENTRY_TYPE_SERIALIZED_LENGTH +
            this.sourceId.serializedLength() +
            this.destinationId.serializedLength()
        );
    }

    toNetworkUpdate(): NetworkUpdate {
        return { type: "LinkRemoved", sourceId: this.sourceId, destinationId: this.destinationId };
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
        .with({ type: "NodeUpdated" }, (update) => new NetworkNodeUpdatedNotificationEntry(update))
        .with({ type: "NodeRemoved" }, (update) => new NetworkNodeRemovedNotificationEntry(update))
        .with({ type: "LinkUpdated" }, (update) => new NetworkLinkUpdatedNotificationEntry(update))
        .with({ type: "LinkRemoved" }, (update) => new NetworkLinkRemovedNotificationEntry(update))
        .exhaustive();
};
