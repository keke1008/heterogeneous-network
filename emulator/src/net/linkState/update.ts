import { Cost, NodeId } from "@core/net/node";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Result } from "oxide.ts";

export interface SerializedStateUpdate {
    nodeAdded: { nodeId: Uint8Array; cost?: Uint8Array }[];
    nodeRemoved: Uint8Array[];
    nodeCostChanged: { nodeId: Uint8Array; cost: Uint8Array }[];

    linkAdded: { nodeId1: Uint8Array; nodeId2: Uint8Array; cost: Uint8Array }[];
    linkRemoved: { nodeId1: Uint8Array; nodeId2: Uint8Array }[];
    linkCostChanged: { nodeId1: Uint8Array; nodeId2: Uint8Array; cost: Uint8Array }[];
}

export class StateUpdate {
    nodeAdded: { nodeId: NodeId; cost?: Cost }[];
    nodeRemoved: NodeId[];
    nodeCostChanged: { nodeId: NodeId; cost: Cost }[];
    linkAdded: { nodeId1: NodeId; nodeId2: NodeId; cost: Cost }[];
    linkRemoved: { nodeId1: NodeId; nodeId2: NodeId }[];
    linkCostChanged: { nodeId1: NodeId; nodeId2: NodeId; cost: Cost }[];

    constructor(args?: {
        nodeAdded?: { nodeId: NodeId; cost?: Cost }[];
        nodeRemoved?: NodeId[];
        nodeCostChanged?: { nodeId: NodeId; cost: Cost }[];
        linkAdded?: { nodeId1: NodeId; nodeId2: NodeId; cost: Cost }[];
        linkRemoved?: { nodeId1: NodeId; nodeId2: NodeId }[];
        linkCostChanged?: { nodeId1: NodeId; nodeId2: NodeId; cost: Cost }[];
    }) {
        this.nodeAdded = args?.nodeAdded ?? [];
        this.nodeRemoved = args?.nodeRemoved ?? [];
        this.nodeCostChanged = args?.nodeCostChanged ?? [];
        this.linkAdded = args?.linkAdded ?? [];
        this.linkRemoved = args?.linkRemoved ?? [];
        this.linkCostChanged = args?.linkCostChanged ?? [];
    }

    static deserialize(serialized: SerializedStateUpdate): StateUpdate {
        const deserialize = <T>(
            value: { deserialize: (arg0: BufferReader) => Result<T, unknown> },
            buffer: Uint8Array,
        ): T => {
            return value.deserialize(new BufferReader(buffer)).unwrap();
        };

        return new StateUpdate({
            nodeAdded: serialized.nodeAdded.map((node) => ({
                nodeId: deserialize(NodeId, node.nodeId),
                cost: node.cost && deserialize(Cost, node.cost),
            })),
            nodeRemoved: serialized.nodeRemoved.map((node) => deserialize(NodeId, node)),
            nodeCostChanged: serialized.nodeCostChanged.map((node) => ({
                nodeId: deserialize(NodeId, node.nodeId),
                cost: deserialize(Cost, node.cost),
            })),
            linkAdded: serialized.linkAdded.map((link) => ({
                nodeId1: deserialize(NodeId, link.nodeId1),
                nodeId2: deserialize(NodeId, link.nodeId2),
                cost: deserialize(Cost, link.cost),
            })),
            linkRemoved: serialized.linkRemoved.map((link) => ({
                nodeId1: deserialize(NodeId, link.nodeId1),
                nodeId2: deserialize(NodeId, link.nodeId2),
            })),
            linkCostChanged: serialized.linkCostChanged.map((link) => ({
                nodeId1: deserialize(NodeId, link.nodeId1),
                nodeId2: deserialize(NodeId, link.nodeId2),
                cost: deserialize(Cost, link.cost),
            })),
        });
    }

    serialize(): SerializedStateUpdate {
        const serialize = (value: { serializedLength: () => number; serialize: (arg0: BufferWriter) => void }) => {
            const writer = new BufferWriter(new Uint8Array(value.serializedLength()));
            value.serialize(writer);
            return writer.unwrapBuffer();
        };

        return {
            nodeAdded: this.nodeAdded.map((node) => ({
                nodeId: serialize(node.nodeId),
                cost: node.cost && serialize(node.cost),
            })),
            nodeRemoved: this.nodeRemoved.map((node) => serialize(node)),
            nodeCostChanged: this.nodeCostChanged.map((node) => ({
                nodeId: serialize(node.nodeId),
                cost: serialize(node.cost),
            })),
            linkAdded: this.linkAdded.map((link) => ({
                nodeId1: serialize(link.nodeId1),
                nodeId2: serialize(link.nodeId2),
                cost: serialize(link.cost),
            })),
            linkRemoved: this.linkRemoved.map((link) => ({
                nodeId1: serialize(link.nodeId1),
                nodeId2: serialize(link.nodeId2),
            })),
            linkCostChanged: this.linkCostChanged.map((link) => ({
                nodeId1: serialize(link.nodeId1),
                nodeId2: serialize(link.nodeId2),
                cost: serialize(link.cost),
            })),
        };
    }

    static merge(...updates: StateUpdate[]): StateUpdate {
        return new StateUpdate({
            nodeAdded: updates.flatMap((update) => update.nodeAdded),
            nodeRemoved: updates.flatMap((update) => update.nodeRemoved),
            nodeCostChanged: updates.flatMap((update) => update.nodeCostChanged),
            linkAdded: updates.flatMap((update) => update.linkAdded),
            linkRemoved: updates.flatMap((update) => update.linkRemoved),
            linkCostChanged: updates.flatMap((update) => update.linkCostChanged),
        });
    }
}
