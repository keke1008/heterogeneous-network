import { Cost, NetworkTopologyUpdate, NodeId, Source } from "@core/net/node";
import { EmptySerdeable, ObjectSerdeable, TransformSerdeable, VariantSerdeable, VectorSerdeable } from "@core/serde";
import { FrameType } from "./common";
import { match } from "ts-pattern";

export type NetworkUpdate = NetworkTopologyUpdate | { type: "FrameReceived"; receivedNodeId: NodeId };

export const NetworkUpdate = {
    isTopologyUpdate(update: NetworkUpdate): update is NetworkTopologyUpdate {
        return update.type !== "FrameReceived";
    },
    isFrameReceived(update: NetworkUpdate): update is NetworkUpdate & { type: "FrameReceived" } {
        return update.type === "FrameReceived";
    },
    intoNotificationEntry(update: NetworkUpdate): NetworkNotificationEntry {
        return match(update)
            .with({ type: "NodeUpdated" }, (update) => new NetworkNodeUpdatedNotificationEntry(update))
            .with({ type: "NodeRemoved" }, (update) => new NetworkNodeRemovedNotificationEntry(update))
            .with({ type: "LinkUpdated" }, (update) => new NetworkLinkUpdatedNotificationEntry(update))
            .with({ type: "LinkRemoved" }, (update) => new NetworkLinkRemovedNotificationEntry(update))
            .with({ type: "FrameReceived" }, (update) => new NetworkFrameReceivedNotificationEntry(update))
            .exhaustive();
    },
};

enum NetworkNotificationEntryType {
    NodeUpdated = 1,
    NodeRemoved = 2,
    LinkUpdated = 3,
    LinkRemoved = 4,
    FrameReceived = 5,
}

export class NetworkNodeUpdatedNotificationEntry {
    readonly entryType = NetworkNotificationEntryType.NodeUpdated as const;
    node: Source;
    cost: Cost;

    constructor(args: { node: Source; cost: Cost }) {
        this.node = args.node;
        this.cost = args.cost;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ node: Source.serdeable, cost: Cost.serdeable }),
        (obj) => new NetworkNodeUpdatedNotificationEntry(obj),
        (frame) => frame,
    );

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

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ id: NodeId.serdeable }),
        (obj) => new NetworkNodeRemovedNotificationEntry(obj),
        (frame) => frame,
    );

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

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            source: Source.serdeable,
            sourceCost: Cost.serdeable,
            destination: Source.serdeable,
            destinationCost: Cost.serdeable,
            linkCost: Cost.serdeable,
        }),
        (obj) => new NetworkLinkUpdatedNotificationEntry(obj),
        (frame) => frame,
    );

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

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ sourceId: NodeId.serdeable, destinationId: NodeId.serdeable }),
        (obj) => new NetworkLinkRemovedNotificationEntry(obj),
        (frame) => frame,
    );

    toNetworkUpdate(): NetworkUpdate {
        return { type: "LinkRemoved", sourceId: this.sourceId, destinationId: this.destinationId };
    }
}

export class NetworkFrameReceivedNotificationEntry {
    readonly entryType = NetworkNotificationEntryType.FrameReceived as const;
    receivedNodeId: NodeId;

    constructor(args: { receivedNodeId: NodeId }) {
        this.receivedNodeId = args.receivedNodeId;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ receivedNodeId: NodeId.serdeable }),
        (obj) => new NetworkFrameReceivedNotificationEntry(obj),
        (frame) => frame,
    );

    toNetworkUpdate(): NetworkUpdate {
        return { type: "FrameReceived", receivedNodeId: this.receivedNodeId };
    }
}

export type NetworkNotificationEntry =
    | NetworkNodeUpdatedNotificationEntry
    | NetworkNodeRemovedNotificationEntry
    | NetworkLinkUpdatedNotificationEntry
    | NetworkLinkRemovedNotificationEntry
    | NetworkFrameReceivedNotificationEntry;

export const NetworkNotificationEntry = {
    serdeable: new VariantSerdeable(
        [
            NetworkNodeUpdatedNotificationEntry.serdeable,
            NetworkNodeRemovedNotificationEntry.serdeable,
            NetworkLinkUpdatedNotificationEntry.serdeable,
            NetworkLinkRemovedNotificationEntry.serdeable,
            NetworkFrameReceivedNotificationEntry.serdeable,
        ],
        (entry) => entry.entryType,
    ),
};

export class NetworkNotificationFrame {
    readonly frameType = FrameType.NetworkNotification as const;
    entries: NetworkNotificationEntry[];

    constructor(entries: NetworkNotificationEntry[]) {
        this.entries = entries;
    }

    static readonly serdeable = new TransformSerdeable(
        new VectorSerdeable(NetworkNotificationEntry.serdeable),
        (obj) => new NetworkNotificationFrame(obj),
        (frame) => frame.entries,
    );
}

export class NetworkSubscriptionFrame {
    readonly frameType = FrameType.NetworkSubscription as const;

    static readonly serdeable = new TransformSerdeable(
        new EmptySerdeable(),
        () => new NetworkSubscriptionFrame(),
        () => undefined,
    );
}

export type NetworkFrame = NetworkNotificationFrame | NetworkSubscriptionFrame;
