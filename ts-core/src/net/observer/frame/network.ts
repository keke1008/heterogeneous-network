import { Cost, NetworkTopologyUpdate, NodeId, PartialNode } from "@core/net/node";
import {
    EmptySerdeable,
    ObjectSerdeable,
    OptionalSerdeable,
    TransformSerdeable,
    VariantSerdeable,
    VectorSerdeable,
} from "@core/serde";
import { FrameType } from "./common";
import { match } from "ts-pattern";
import { ClusterId, OptionalClusterId } from "@core/net/node/clusterId";

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
            .with({ type: "NodeUpdated" }, ({ node }) => new NetworkNodeUpdatedNotificationEntry(node))
            .with({ type: "NodeRemoved" }, (update) => new NetworkNodeRemovedNotificationEntry(update))
            .with(
                { type: "LinkUpdated" },
                (update) =>
                    new NetworkLinkUpdatedNotificationEntry({
                        sourceNodeId: update.source.nodeId,
                        destinationNodeId: update.destination.nodeId,
                        linkCost: update.linkCost,
                    }),
            )
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
    nodeId: NodeId;
    cost: Cost | undefined;
    clusterId: OptionalClusterId | undefined;

    constructor(node: PartialNode) {
        this.nodeId = node.nodeId;
        this.cost = node.cost;
        this.clusterId = node.clusterId;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            nodeId: NodeId.serdeable,
            cost: new OptionalSerdeable(Cost.serdeable),
            clusterId: new OptionalSerdeable(ClusterId.serdeable),
        }),
        (obj) => new NetworkNodeUpdatedNotificationEntry(obj),
        (frame) => frame,
    );

    toNetworkUpdate(): NetworkUpdate {
        return {
            type: "NodeUpdated",
            node: { nodeId: this.nodeId, cost: this.cost, clusterId: this.clusterId },
        };
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
    sourceNodeId: NodeId;
    destinationNodeId: NodeId;
    linkCost: Cost;

    constructor(args: { sourceNodeId: NodeId; destinationNodeId: NodeId; linkCost: Cost }) {
        this.sourceNodeId = args.sourceNodeId;
        this.destinationNodeId = args.destinationNodeId;
        this.linkCost = args.linkCost;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            sourceNodeId: NodeId.serdeable,
            destinationNodeId: NodeId.serdeable,
            linkCost: Cost.serdeable,
        }),
        (obj) => new NetworkLinkUpdatedNotificationEntry(obj),
        (frame) => frame,
    );

    toNetworkUpdate(): NetworkUpdate {
        return {
            type: "LinkUpdated",
            source: { nodeId: this.sourceNodeId },
            destination: { nodeId: this.destinationNodeId },
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
