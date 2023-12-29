import { ClusterId, NodeId } from "../node";
import { TransformSerdeable, TupleSerdeable } from "@core/serde";
import { OptionalClusterId } from "./clusterId";

export class Destination {
    nodeId: NodeId;
    clusterId: OptionalClusterId;

    constructor(args?: { nodeId?: NodeId; clusterId: OptionalClusterId }) {
        this.nodeId = args?.nodeId ?? NodeId.broadcast();
        this.clusterId = args?.clusterId ?? ClusterId.noCluster();
    }

    static broadcast(): Destination {
        return new Destination();
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

    toUniqueString(): string {
        return `${this.nodeId.toString()}+${this.clusterId.toUniqueString()}`;
    }

    static readonly serdeable = new TransformSerdeable(
        new TupleSerdeable([NodeId.serdeable, ClusterId.serdeable] as const),
        ([nodeId, clusterId]) => new Destination({ nodeId, clusterId }),
        (destination) => [destination.nodeId, destination.clusterId] as const,
    );

    display(): string {
        return `Destination(${this.nodeId?.display()}, ${this.clusterId?.display()})`;
    }
}
