import { ClusterId, NodeId } from "../node";
import { TransformSerdeable, TupleSerdeable } from "@core/serde";
import { OptionalClusterId } from "./clusterId";
import { UniqueKey } from "@core/object";

export class Destination implements UniqueKey {
    nodeId: NodeId;
    clusterId: OptionalClusterId;

    constructor(args?: { nodeId?: NodeId; clusterId: OptionalClusterId }) {
        this.nodeId = args?.nodeId ?? NodeId.broadcast();
        this.clusterId = args?.clusterId ?? ClusterId.noCluster();
    }

    static broadcast(): Destination {
        return new Destination();
    }

    static fromNodeId(nodeId: NodeId): Destination {
        return new Destination({ nodeId, clusterId: ClusterId.noCluster() });
    }

    isBroadcast(): boolean {
        return this.nodeId.isBroadcast() && this.clusterId.isNoCluster();
    }

    isUnicast(): boolean {
        return !this.nodeId.isBroadcast();
    }

    isMulticast(): boolean {
        return this.nodeId.isBroadcast() && !this.clusterId.isNoCluster();
    }

    equals(other: Destination): boolean {
        return this.nodeId.equals(other.nodeId) && this.clusterId.equals(other.clusterId);
    }

    static readonly serdeable = new TransformSerdeable(
        new TupleSerdeable([NodeId.serdeable, ClusterId.serdeable] as const),
        ([nodeId, clusterId]) => new Destination({ nodeId, clusterId }),
        (destination) => [destination.nodeId, destination.clusterId] as const,
    );

    toString(): string {
        return `Destination(${this.nodeId}, ${this.clusterId})`;
    }

    display(): string {
        return `${this.nodeId.display()} ${this.clusterId.display()}`;
    }

    uniqueKey(): string {
        return `${this.nodeId.uniqueKey()}+${this.clusterId.uniqueKey()}`;
    }

    toJSON(): string {
        return this.toString();
    }
}
