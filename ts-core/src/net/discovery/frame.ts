import { DeserializeEnum, DeserializeResult, DeserializeU8, SerializeEnum, SerializeU8 } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { Cost, Destination, NodeId, Source } from "../node";
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
    source: Source;
    target: Destination;
    sender: Source;

    private constructor(args: {
        type: DiscoveryFrameType;
        frameId: FrameId;
        totalCost: Cost;
        source: Source;
        target: Destination;
        sender: Source;
    }) {
        this.type = args.type;
        this.frameId = args.frameId;
        this.totalCost = args.totalCost;
        this.source = args.source;
        this.target = args.target;
        this.sender = args.sender;
    }

    static deserialize(reader: BufferReader): DeserializeResult<DiscoveryFrame> {
        return frameTypeDeserializer.deserialize(reader).andThen((type) => {
            return FrameId.deserialize(reader).andThen((frameId) => {
                return Cost.deserialize(reader).andThen((totalCost) => {
                    return Source.deserialize(reader).andThen((sourceId) => {
                        return Destination.deserialize(reader).andThen((target) => {
                            return Source.deserialize(reader).map((senderId) => {
                                return new DiscoveryFrame({
                                    type,
                                    frameId,
                                    totalCost,
                                    source: sourceId,
                                    target,
                                    sender: senderId,
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
        this.source.serialize(writer);
        this.target.serialize(writer);
        this.sender.serialize(writer);
    }

    serializedLength(): number {
        const serializers = [
            frameTypeSerializer(this.type),
            this.frameId,
            this.totalCost,
            this.source,
            this.target,
            this.sender,
        ];
        return serializers.reduce((acc, s) => acc + s.serializedLength(), 0);
    }

    static request(args: { frameId: FrameId; destination: Destination; local: Source }) {
        return new DiscoveryFrame({
            type: DiscoveryFrameType.Request,
            frameId: args.frameId,
            totalCost: new Cost(0),
            source: args.local,
            target: args.destination,
            sender: args.local,
        });
    }

    repeat(args: { local: Source; sourceLinkCost: Cost; localCost: Cost }) {
        return new DiscoveryFrame({
            type: this.type,
            frameId: this.frameId,
            totalCost: this.totalCost.add(args.sourceLinkCost).add(args.localCost),
            source: this.source,
            target: this.target,
            sender: args.local,
        });
    }

    reply(args: { local: Source; frameId: FrameId }) {
        if (this.type !== DiscoveryFrameType.Request) {
            throw new Error("Cannot reply to a non-request frame");
        }
        return new DiscoveryFrame({
            type: DiscoveryFrameType.Response,
            frameId: args.frameId,
            totalCost: new Cost(0),
            source: this.source,
            target: this.target,
            sender: args.local,
        });
    }

    destination(): Destination | NodeId {
        return this.type === DiscoveryFrameType.Request ? this.target : this.source.intoDestination();
    }

    destinationId(): NodeId | undefined {
        return this.type === DiscoveryFrameType.Request ? this.target.nodeId : this.source.nodeId;
    }
}
