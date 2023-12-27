import { DeserializeEnum, DeserializeResult, DeserializeU8, SerializeEnum, SerializeU8 } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { Cost, Destination, NodeId, Source } from "../node";
import { FrameId } from "./frameId";
import { NeighborFrame } from "../neighbor";

export enum DiscoveryFrameType {
    Request = 1,
    Response = 2,
}

const frameTypeSerializer = (frameType: DiscoveryFrameType) => new SerializeEnum(frameType, SerializeU8);
const frameTypeDeserializer = new DeserializeEnum(DiscoveryFrameType, DeserializeU8);

export class TotalCost {
    #cost: Cost;

    constructor(cost: Cost) {
        this.#cost = cost;
    }

    get(): Cost {
        return this.#cost;
    }
}

interface DiscoveryFrameArgs {
    type: DiscoveryFrameType;
    frameId: FrameId;
    totalCost: Cost;
    source: Source;
    target: Destination;
}

export class DiscoveryFrame {
    type: DiscoveryFrameType;
    frameId: FrameId;
    totalCost: Cost;
    source: Source;
    target: Destination;

    protected constructor(args: DiscoveryFrameArgs) {
        this.type = args.type;
        this.frameId = args.frameId;
        this.totalCost = args.totalCost;
        this.source = args.source;
        this.target = args.target;
    }

    calculateTotalCost(linkCost: Cost, localCost: Cost): TotalCost {
        return new TotalCost(this.totalCost.add(linkCost).add(localCost));
    }

    static deserialize(reader: BufferReader): DeserializeResult<DiscoveryFrame> {
        return frameTypeDeserializer.deserialize(reader).andThen((type) => {
            return FrameId.deserialize(reader).andThen((frameId) => {
                return Cost.deserialize(reader).andThen((totalCost) => {
                    return Source.deserialize(reader).andThen((source) => {
                        return Destination.deserialize(reader).map((target) => {
                            return new DiscoveryFrame({
                                type,
                                frameId,
                                totalCost,
                                source: source,
                                target,
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
    }

    serializedLength(): number {
        const serializers = [frameTypeSerializer(this.type), this.frameId, this.totalCost, this.source, this.target];
        return serializers.reduce((acc, s) => acc + s.serializedLength(), 0);
    }

    static request(args: { frameId: FrameId; destination: Destination; local: Source }) {
        return new DiscoveryFrame({
            type: DiscoveryFrameType.Request,
            frameId: args.frameId,
            totalCost: new Cost(0),
            source: args.local,
            target: args.destination,
        });
    }

    repeat(args: { sourceLinkCost: Cost; localCost: Cost }) {
        return new DiscoveryFrame({
            type: this.type,
            frameId: this.frameId,
            totalCost: this.totalCost.add(args.sourceLinkCost).add(args.localCost),
            source: this.source,
            target: this.target,
        });
    }

    reply(args: { frameId: FrameId }) {
        if (this.type !== DiscoveryFrameType.Request) {
            throw new Error("Cannot reply to a non-request frame");
        }
        return new DiscoveryFrame({
            type: DiscoveryFrameType.Response,
            frameId: args.frameId,
            totalCost: new Cost(0),
            source: this.source,
            target: this.target,
        });
    }

    replyByCache(args: { frameId: FrameId; cache: TotalCost }) {
        if (this.type !== DiscoveryFrameType.Request) {
            throw new Error("Cannot reply to a non-request frame");
        }
        return new DiscoveryFrame({
            type: DiscoveryFrameType.Response,
            frameId: args.frameId,
            totalCost: args.cache.get(),
            source: this.source,
            target: this.target,
        });
    }

    destination(): Destination {
        return this.type === DiscoveryFrameType.Request ? this.target : this.source.intoDestination();
    }

    destinationId(): NodeId | undefined {
        return this.type === DiscoveryFrameType.Request ? this.target.nodeId : this.source.nodeId;
    }

    display(): string {
        const type = this.type === DiscoveryFrameType.Request ? "REQ" : "RES";
        return `${type} ${this.frameId.display()} ${this.totalCost.display()} ${this.source.display()} ${this.target.display()}`;
    }
}

export class ReceivedDiscoveryFrame extends DiscoveryFrame {
    previousHop: Source;

    private constructor(args: DiscoveryFrameArgs & { sender: Source }) {
        super(args);
        this.previousHop = args.sender;
    }

    static deserializeFromNeighborFrame(neighborFrame: NeighborFrame): DeserializeResult<ReceivedDiscoveryFrame> {
        return DiscoveryFrame.deserialize(neighborFrame.reader).map((frame) => {
            return new ReceivedDiscoveryFrame({
                type: frame.type,
                frameId: frame.frameId,
                totalCost: frame.totalCost,
                source: frame.source,
                target: frame.target,
                sender: neighborFrame.sender,
            });
        });
    }
}
