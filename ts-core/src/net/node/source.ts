import { DeserializeResult } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { ClusterId, NoCluster } from "./clusterId";
import { NodeId } from "./nodeId";
import { Destination } from "./destination";

export class Source {
    #nodeId: NodeId;
    #clusterId: ClusterId | NoCluster;

    constructor(args: { nodeId: NodeId; clusterId?: ClusterId | NoCluster }) {
        this.#nodeId = args.nodeId;
        this.#clusterId = args.clusterId ?? ClusterId.noCluster();
    }

    get nodeId(): NodeId {
        return this.#nodeId;
    }

    get clusterId(): ClusterId | NoCluster {
        return this.#clusterId;
    }

    equals(other: Source): boolean {
        return this.#nodeId.equals(other.#nodeId) && this.#clusterId.equals(other.#clusterId);
    }

    intoDestination(): Destination {
        return new Destination({
            nodeId: this.#nodeId,
            clusterId: this.#clusterId instanceof NoCluster ? undefined : this.#clusterId,
        });
    }

    static fromDestination(destination: Destination): Source | undefined {
        if (destination.nodeId === undefined) {
            return undefined;
        }
        return new Source({ nodeId: destination.nodeId, clusterId: destination.clusterId ?? ClusterId.noCluster() });
    }

    static deserialize(reader: BufferReader): DeserializeResult<Source> {
        return NodeId.deserialize(reader).andThen((nodeId) => {
            return ClusterId.deserialize(reader).map((clusterId) => {
                return new Source({ nodeId, clusterId });
            });
        });
    }

    serialize(writer: BufferWriter) {
        this.#nodeId.serialize(writer);
        this.#clusterId.serialize(writer);
    }

    serializedLength(): number {
        return this.#nodeId.serializedLength() + this.#clusterId.serializedLength();
    }

    display(): string {
        return `Source(${this.#nodeId.display()}, ${this.#clusterId.display()})`;
    }
}
