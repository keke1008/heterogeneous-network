import { P, match } from "ts-pattern";
import { ClusterId, NodeId } from "../node";
import { DeserializeResult, DeserializeVariant, SerializeEmpty, SerializeTuple, SerializeVariant } from "@core/serde";
import { Ok } from "oxide.ts";
import { BufferReader, BufferWriter } from "../buffer";
import { NoCluster } from "./clusterId";

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

    equals(other: Destination): boolean {
        const equalNodeId = (other.nodeId && this.nodeId?.equals(other.nodeId)) === true;
        const equalClusterId = (other.clusterId && this.clusterId?.equals(other.clusterId)) === true;
        return equalNodeId && equalClusterId;
    }

    hasOnlyClusterId(): this is { clusterId: ClusterId } {
        return this.nodeId === undefined && this.clusterId !== undefined;
    }

    hasOnlyNodeId(): this is { nodeId: NodeId } {
        return this.nodeId !== undefined && this.clusterId === undefined;
    }

    matches(nodeId: NodeId, clusterId: ClusterId | NoCluster): boolean {
        if (this.nodeId && !this.nodeId.equals(nodeId)) {
            return false;
        }
        if (this.clusterId && this.clusterId.equals(clusterId)) {
            return false;
        }
        return true;
    }

    toUniqueString(): string {
        return `${this.nodeId?.toString()}+${this.clusterId?.toUniqueString()}`;
    }

    static #deserializer = new DeserializeVariant(
        { deserialize: () => Ok({ nodeId: undefined, clusterId: undefined } as const) },
        { deserialize: (reader) => NodeId.deserialize(reader).map((nodeId) => ({ nodeId })) },
        {
            deserialize: (reader) => {
                return ClusterId.noClusterExcludedDeserializer.deserialize(reader).map((clusterId) => ({ clusterId }));
            },
        },
        {
            deserialize: (reader) => {
                return NodeId.deserialize(reader).andThen((nodeId) => {
                    return ClusterId.noClusterExcludedDeserializer.deserialize(reader).map((clusterId) => {
                        return { nodeId, clusterId };
                    });
                });
            },
        },
    );

    static deserialize(reader: BufferReader): DeserializeResult<Destination> {
        return this.#deserializer.deserialize(reader).map((value) => new Destination(value));
    }

    static #serializer = (nodeId?: NodeId, clusterId?: ClusterId) => {
        const [index, serializer] = match({ nodeId, clusterId })
            .with({ nodeId: undefined, clusterId: undefined }, () => [1, new SerializeEmpty()] as const)
            .with({ nodeId: P.not(undefined), clusterId: undefined }, (v) => [2, v.nodeId] as const)
            .with({ nodeId: undefined, clusterId: P.not(undefined) }, (v) => [3, v.clusterId] as const)
            .with(
                { nodeId: P.not(undefined), clusterId: P.not(undefined) },
                (v) => [4, new SerializeTuple(v.nodeId, v.clusterId)] as const,
            )
            .exhaustive();
        return new SerializeVariant(index, serializer);
    };

    serialize(writer: BufferWriter) {
        return Destination.#serializer(this.nodeId, this.clusterId).serialize(writer);
    }

    serializedLength(): number {
        return Destination.#serializer(this.nodeId, this.clusterId).serializedLength();
    }
}
