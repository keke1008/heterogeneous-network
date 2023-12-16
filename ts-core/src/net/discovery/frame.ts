import { DeserializeEnum, DeserializeResult, DeserializeU8, SerializeEnum, SerializeU8 } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { Cost, NodeId } from "../node";
import { FrameId } from "./frameId";

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
    destinationId: NodeId;
    senderId: NodeId;

    private constructor(args: {
        type: DiscoveryFrameType;
        frameId: FrameId;
        totalCost: Cost;
        sourceId: NodeId;
        destinationId: NodeId;
        senderId: NodeId;
    }) {
        this.type = args.type;
        this.frameId = args.frameId;
        this.totalCost = args.totalCost;
        this.sourceId = args.sourceId;
        this.destinationId = args.destinationId;
        this.senderId = args.senderId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<DiscoveryFrame> {
        return frameTypeDeserializer.deserialize(reader).andThen((type) => {
            return FrameId.deserialize(reader).andThen((frameId) => {
                return Cost.deserialize(reader).andThen((totalCost) => {
                    return NodeId.deserialize(reader).andThen((sourceId) => {
                        return NodeId.deserialize(reader).andThen((destinationId) => {
                            return NodeId.deserialize(reader).map((senderId) => {
                                return new DiscoveryFrame({
                                    type,
                                    frameId,
                                    totalCost,
                                    sourceId,
                                    destinationId,
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
        this.destinationId.serialize(writer);
        this.senderId.serialize(writer);
    }

    serializedLength(): number {
        const serializers = [
            frameTypeSerializer(this.type),
            this.frameId,
            this.totalCost,
            this.sourceId,
            this.destinationId,
            this.senderId,
        ];
        return serializers.reduce((acc, s) => acc + s.serializedLength(), 0);
    }

    static request(args: { frameId: FrameId; destinationId: NodeId; localId: NodeId }) {
        return new DiscoveryFrame({
            type: DiscoveryFrameType.Request,
            frameId: args.frameId,
            totalCost: new Cost(0),
            sourceId: args.localId,
            destinationId: args.destinationId,
            senderId: args.localId,
        });
    }

    repeat(args: { localId: NodeId; sourceLinkCost: Cost; localCost: Cost }) {
        return new DiscoveryFrame({
            type: this.type,
            frameId: this.frameId,
            totalCost: this.totalCost.add(args.sourceLinkCost).add(args.localCost),
            sourceId: this.sourceId,
            destinationId: this.destinationId,
            senderId: args.localId,
        });
    }

    reply(args: { localId: NodeId; frameId: FrameId }) {
        return new DiscoveryFrame({
            type: DiscoveryFrameType.Response,
            frameId: args.frameId,
            totalCost: new Cost(0),
            sourceId: this.destinationId,
            destinationId: this.sourceId,
            senderId: args.localId,
        });
    }
}
