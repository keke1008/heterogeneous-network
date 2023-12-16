import { match } from "ts-pattern";
import { ClusterId, NodeId } from "../node";
import { DeserializeVariant, SerializeEmpty, SerializeTuple, SerializeVariant } from "@core/serde";
import { Ok } from "oxide.ts";
import { BufferReader, BufferWriter } from "../buffer";

type DestinationValue =
    | { type: "broadcast" }
    | { type: "nodeId"; nodeId: NodeId }
    | { type: "clusterId"; clusterId: ClusterId }
    | { type: "nodeIdAndClusterId"; nodeId: NodeId; clusterId: ClusterId };

const DestinationValue = {
    deserializer: new DeserializeVariant(
        { deserialize: () => Ok({ type: "broadcast" } as const) },
        { deserialize: (reader) => NodeId.deserialize(reader).map((nodeId) => ({ type: "nodeId", nodeId }) as const) },
        {
            deserialize: (reader) => {
                return ClusterId.deserialize(reader).map((clusterId) => ({ type: "clusterId", clusterId }) as const);
            },
        },
        {
            deserialize: (reader) => {
                return NodeId.deserialize(reader).andThen((nodeId) => {
                    return ClusterId.deserialize(reader).map((clusterId) => {
                        return { type: "nodeIdAndClusterId", nodeId, clusterId } as const;
                    });
                });
            },
        },
    ),

    serializer: (value: DestinationValue) => {
        const [index, serializer] = match(value)
            .with({ type: "broadcast" }, () => [1, new SerializeEmpty()] as const)
            .with({ type: "nodeId" }, (v) => [2, v.nodeId] as const)
            .with({ type: "clusterId" }, (v) => [3, v.clusterId] as const)
            .with({ type: "nodeIdAndClusterId" }, (v) => [4, new SerializeTuple(v.nodeId, v.clusterId)] as const)
            .exhaustive();
        return new SerializeVariant(index, serializer);
    },
};

export class Destination {
    #value: DestinationValue;

    private constructor(value: DestinationValue) {
        this.#value = value;
    }

    static broadcast(): Destination {
        return new Destination({ type: "broadcast" });
    }

    static nodeId(nodeId: NodeId): Destination {
        return new Destination({ type: "nodeId", nodeId });
    }

    static clusterId(clusterId: ClusterId): Destination {
        return new Destination({ type: "clusterId", clusterId });
    }

    static nodeIdAndClusterId(nodeId: NodeId, clusterId: ClusterId): Destination {
        return new Destination({ type: "nodeIdAndClusterId", nodeId, clusterId });
    }

    isBroadcast(): boolean {
        return this.#value.type === "broadcast";
    }

    equals(other: Destination): boolean {
        return match({ this: this.#value, other: other.#value })
            .with({ this: { type: "broadcast" }, other: { type: "broadcast" } }, () => true)
            .with(
                { this: { type: "nodeId" }, other: { type: "nodeId" } },
                ({ this: { nodeId: a }, other: { nodeId: b } }) => a.equals(b),
            )
            .with(
                { this: { type: "clusterId" }, other: { type: "clusterId" } },
                ({ this: { clusterId: a }, other: { clusterId: b } }) => a.equals(b),
            )
            .with(
                { this: { type: "nodeIdAndClusterId" }, other: { type: "nodeIdAndClusterId" } },
                ({ this: { nodeId: a, clusterId: b }, other: { nodeId: c, clusterId: d } }) =>
                    a.equals(c) && b.equals(d),
            )
            .otherwise(() => false);
    }

    matches(nodeId: NodeId, clusterId: ClusterId): boolean {
        return match(this.#value)
            .with({ type: "broadcast" }, () => true)
            .with({ type: "nodeId" }, ({ nodeId: a }) => a.equals(nodeId))
            .with({ type: "clusterId" }, ({ clusterId: a }) => a.equals(clusterId))
            .with(
                { type: "nodeIdAndClusterId" },
                ({ nodeId: a, clusterId: b }) => a.equals(nodeId) && b.equals(clusterId),
            )
            .exhaustive();
    }

    nodeId(): NodeId | undefined {
        return match(this.#value)
            .with({ type: "nodeId" }, ({ nodeId }) => nodeId)
            .with({ type: "nodeIdAndClusterId" }, ({ nodeId }) => nodeId)
            .otherwise(() => undefined);
    }

    clusterId(): ClusterId | undefined {
        return match(this.#value)
            .with({ type: "clusterId" }, ({ clusterId }) => clusterId)
            .with({ type: "nodeIdAndClusterId" }, ({ clusterId }) => clusterId)
            .otherwise(() => undefined);
    }

    static deserialize(reader: BufferReader) {
        return DestinationValue.deserializer.deserialize(reader).map((value) => new Destination(value));
    }

    serialize(writer: BufferWriter) {
        return DestinationValue.serializer(this.#value).serialize(writer);
    }

    serializedLength(): number {
        return DestinationValue.serializer(this.#value).serializedLength();
    }

    toUniqueString(): string {
        return match(this.#value)
            .with({ type: "broadcast" }, (v) => v.type)
            .with({ type: "nodeId" }, ({ type, nodeId }) => `${type}(${nodeId.toString()})`)
            .with({ type: "clusterId" }, ({ type, clusterId }) => `${type}(${clusterId.toString()})`)
            .with(
                { type: "nodeIdAndClusterId" },
                ({ type, nodeId, clusterId }) => `${type}(${nodeId.toString()}(${clusterId.toUniqueString()}))`,
            )
            .exhaustive();
    }
}
