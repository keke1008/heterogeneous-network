import { ClusterId, NodeId } from "../node";
import { TransformSerdeable } from "@core/serde";
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
        return this.nodeId === undefined && this.clusterId === undefined;
    }

    isUnicast(): boolean {
        return this.nodeId !== undefined;
    }

    isMulticast(): boolean {
        return this.nodeId === undefined && this.clusterId !== undefined;
    }

    equals(other: Destination): boolean {
        return this.nodeId.equals(other.nodeId) && this.clusterId.equals(other.clusterId);
    }

    toUniqueString(): string {
        return `${this.nodeId.toString()}+${this.clusterId.toUniqueString()}`;
    }

    // TODO: Destinationの実装を変えたら、このメソッドも変える
    static readonly serdeable = new TransformSerdeable(
        null!,
        () => new Destination(),
        () => undefined,
    );

    display(): string {
        return `Destination(${this.nodeId?.display()}, ${this.clusterId?.display()})`;
    }
}
