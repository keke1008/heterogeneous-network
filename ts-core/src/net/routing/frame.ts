import { DeserializeResult, RemainingBytesSerdeable, TransformSerdeable, TupleSerdeable } from "@core/serde";
import { FrameId } from "../discovery";
import { Source, Destination, NodeId } from "../node";
import { NeighborFrame } from "../neighbor";
import { BufferReader } from "../buffer";

interface RoutingFrameArgs {
    source: Source;
    destination: Destination;
    frameId: FrameId;
    payload: Uint8Array;
}

export class RoutingFrame {
    source: Source;
    destination: Destination;
    frameId: FrameId;
    payload: Uint8Array;

    constructor(opts: RoutingFrameArgs) {
        this.source = opts.source;
        this.destination = opts.destination;
        this.frameId = opts.frameId;
        this.payload = opts.payload;
    }

    static readonly serdeable = new TransformSerdeable(
        new TupleSerdeable([
            Source.serdeable,
            Destination.serdeable,
            FrameId.serdeable,
            new RemainingBytesSerdeable(),
        ] as const),
        ([source, destination, frameId, payload]) => {
            return new RoutingFrame({ source, destination, frameId, payload });
        },
        (frame) => {
            return [frame.source, frame.destination, frame.frameId, frame.payload] as const;
        },
    );
}

export class ReceivedRoutingFrame extends RoutingFrame {
    previousHop: NodeId;

    constructor(args: RoutingFrameArgs & { previousHop: NodeId }) {
        super(args);
        this.previousHop = args.previousHop;
    }

    static deserializeFromNeighborFrame(frame: NeighborFrame): DeserializeResult<ReceivedRoutingFrame> {
        return RoutingFrame.serdeable
            .deserializer()
            .deserialize(new BufferReader(frame.payload))
            .map((routingFrame) => {
                return new ReceivedRoutingFrame({ previousHop: frame.sender, ...routingFrame });
            });
    }
}
