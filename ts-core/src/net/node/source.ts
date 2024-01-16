import { TransformSerdeable, TupleSerdeable } from "@core/serde";
import { ClusterId, NoCluster } from "./clusterId";
import { NodeId } from "./nodeId";
import { Destination } from "./destination";

export class Source {
    #nodeId: NodeId;
    #clusterId: ClusterId | NoCluster;

    constructor(args: { nodeId: NodeId; clusterId: ClusterId | NoCluster }) {
        if (args.nodeId.isBroadcast()) {
            throw new Error("Source cannot be broadcast");
        }

        this.#nodeId = args.nodeId;
        this.#clusterId = args.clusterId ?? ClusterId.noCluster();
    }

    get nodeId(): NodeId {
        return this.#nodeId;
    }

    get clusterId(): ClusterId | NoCluster {
        return this.#clusterId;
    }

    static broadcast(): Source {
        return new Source({ nodeId: NodeId.broadcast(), clusterId: new NoCluster() });
    }

    static loopback(): Source {
        return new Source({ nodeId: NodeId.loopback(), clusterId: new NoCluster() });
    }

    equals(other: Source): boolean {
        return this.#nodeId.equals(other.#nodeId) && this.#clusterId.equals(other.#clusterId);
    }

    intoDestination(): Destination {
        return new Destination({ nodeId: this.#nodeId, clusterId: this.#clusterId });
    }

    matches(destination: Destination): boolean {
        if (destination.isBroadcast()) {
            return true;
        }

        if (destination.isUnicast()) {
            return this.#nodeId.equals(destination.nodeId);
        }

        return this.clusterId.equals(destination.clusterId);
    }

    static fromDestination(destination: Destination): Source | undefined {
        if (destination.nodeId.isBroadcast()) {
            return undefined;
        }
        return new Source({ nodeId: destination.nodeId, clusterId: destination.clusterId ?? ClusterId.noCluster() });
    }

    static readonly serdeable = new TransformSerdeable(
        new TupleSerdeable([NodeId.serdeable, ClusterId.serdeable] as const),
        ([nodeId, clusterId]) => new Source({ nodeId, clusterId }),
        (source) => [source.#nodeId, source.#clusterId] as const,
    );

    toString(): string {
        return `Source(${this.#nodeId}, ${this.#clusterId})`;
    }

    toJSON(): string {
        return this.toString();
    }
}
