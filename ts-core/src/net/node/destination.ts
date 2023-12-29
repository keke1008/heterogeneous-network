import { ClusterId, NodeId } from "../node";
import { TransformSerdeable } from "@core/serde";

export class Destination {
    nodeId: NodeId | undefined;
    clusterId: ClusterId | undefined;

    constructor(args?: { nodeId?: NodeId; clusterId?: ClusterId }) {
        this.nodeId = args?.nodeId;
        this.clusterId = args?.clusterId;
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
        const equalNodeId = (other.nodeId && this.nodeId?.equals(other.nodeId)) === true;
        const equalClusterId = (other.clusterId && this.clusterId?.equals(other.clusterId)) === true;
        return equalNodeId && equalClusterId;
    }

    toUniqueString(): string {
        return `${this.nodeId?.toString()}+${this.clusterId?.toUniqueString()}`;
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
