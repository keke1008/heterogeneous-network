import { DeserializeEnum, DeserializeResult, DeserializeU8, SerializeEnum, SerializeU8 } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { Cost, NodeId } from "../node";
import { FrameId } from "./frameId";
import { Destination } from "./destination";

export enum DiscoveryFrameType {
    Request = 1,
    Response = 2,
}

const frameTypeSerializer = (frameType: DiscoveryFrameType) => new SerializeEnum(frameType, SerializeU8);
const frameTypeDeserializer = new DeserializeEnum(DiscoveryFrameType, DeserializeU8);

export class DiscoveryFrame {
    type: DiscoveryFrameType;
    frameId: FrameId;
    totalCost: Cost;
    sourceId: NodeId;
    target: Destination;
    senderId: NodeId;

    private constructor(args: {
        type: DiscoveryFrameType;
        frameId: FrameId;
        totalCost: Cost;
        sourceId: NodeId;
        target: Destination;
        senderId: NodeId;
    }) {
        this.type = args.type;
        this.frameId = args.frameId;
        this.totalCost = args.totalCost;
        this.sourceId = args.sourceId;
        this.target = args.target;
        this.senderId = args.senderId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<DiscoveryFrame> {
        return frameTypeDeserializer.deserialize(reader).andThen((type) => {
            return FrameId.deserialize(reader).andThen((frameId) => {
                return Cost.deserialize(reader).andThen((totalCost) => {
                    return NodeId.deserialize(reader).andThen((sourceId) => {
                        return Destination.deserialize(reader).andThen((target) => {
                            return NodeId.deserialize(reader).map((senderId) => {
                                return new DiscoveryFrame({
                                    type,
                                    frameId,
                                    totalCost,
                                    sourceId,
                                    target,
                                    senderId,
                                });
                            });
                        });
                    });
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        frameTypeSerializer(this.type).serialize(writer);
        this.frameId.serialize(writer);
        this.totalCost.serialize(writer);
        this.sourceId.serialize(writer);
        this.target.serialize(writer);
        this.senderId.serialize(writer);
    }

    serializedLength(): number {
        const serializers = [
            frameTypeSerializer(this.type),
            this.frameId,
            this.totalCost,
            this.sourceId,
            this.target,
            this.senderId,
        ];
        return serializers.reduce((acc, s) => acc + s.serializedLength(), 0);
    }

    static request(args: { frameId: FrameId; destination: Destination; localId: NodeId }) {
        return new DiscoveryFrame({
            type: DiscoveryFrameType.Request,
            frameId: args.frameId,
            totalCost: new Cost(0),
            sourceId: args.localId,
            target: args.destination,
            senderId: args.localId,
        });
    }

    repeat(args: { localId: NodeId; sourceLinkCost: Cost; localCost: Cost }) {
        return new DiscoveryFrame({
            type: this.type,
            frameId: this.frameId,
            totalCost: this.totalCost.add(args.sourceLinkCost).add(args.localCost),
            sourceId: this.sourceId,
            target: this.target,
            senderId: args.localId,
        });
    }

    reply(args: { localId: NodeId; frameId: FrameId }) {
        return new DiscoveryFrame({
            type: DiscoveryFrameType.Response,
            frameId: args.frameId,
            totalCost: new Cost(0),
            sourceId: this.sourceId,
            target: this.target,
            senderId: args.localId,
        });
    }

    destination(): Destination | NodeId {
        return this.type === DiscoveryFrameType.Request ? this.target : this.sourceId;
    }

    destinationId(): NodeId | undefined {
        return this.type === DiscoveryFrameType.Request ? this.target.nodeId() : this.sourceId;
    }
}
