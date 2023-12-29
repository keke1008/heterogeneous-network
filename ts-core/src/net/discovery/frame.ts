import { DeserializeResult, EnumSerdeable, TransformSerdeable, TupleSerdeable } from "@core/serde";
import { Cost, Destination, NodeId, Source } from "../node";
import { FrameId } from "./frameId";
import { NeighborFrame } from "../neighbor";
import { BufferReader } from "../buffer";

export enum DiscoveryFrameType {
    Request = 1,
    Response = 2,
}

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

    static readonly serdeable = new TransformSerdeable(
        new TupleSerdeable([
            new EnumSerdeable(DiscoveryFrameType),
            FrameId.serdeable,
            Cost.serdeable,
            Source.serdeable,
            Destination.serdeable,
        ] as const),
        ([type, frameId, totalCost, source, target]) => {
            return new DiscoveryFrame({ type, frameId, totalCost, source, target });
        },
        (frame) => {
            return [frame.type, frame.frameId, frame.totalCost, frame.source, frame.target] as const;
        },
    );

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
        return DiscoveryFrame.serdeable
            .deserializer()
            .deserialize(new BufferReader(neighborFrame.payload))
            .map((frame) => {
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
