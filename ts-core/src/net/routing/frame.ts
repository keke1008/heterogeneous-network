import { DeserializeResult } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { FrameId } from "../discovery";
import { Source, Destination, NodeId } from "../node";
import { NeighborFrame } from "../neighbor";

interface RoutingFrameArgs {
    source: Source;
    destination: Destination;
    frameId: FrameId;
    reader: BufferReader;
}

export class RoutingFrame {
    source: Source;
    destination: Destination;
    frameId: FrameId;
    reader: BufferReader;

    constructor(opts: RoutingFrameArgs) {
        this.source = opts.source;
        this.destination = opts.destination;
        this.frameId = opts.frameId;
        this.reader = opts.reader;
    }

    repeat(): RoutingFrame {
        return new RoutingFrame({
            source: this.source,
            destination: this.destination,
            frameId: this.frameId,
            reader: this.reader.initialized(),
        });
    }

    static deserialize(reader: BufferReader): DeserializeResult<RoutingFrame> {
        return Source.deserialize(reader).andThen((source) => {
            return Destination.deserialize(reader).andThen((destination) => {
                return FrameId.deserialize(reader).map((frameId) => {
                    return new RoutingFrame({
                        source,
                        destination,
                        frameId,
                        reader: reader.subReader(),
                    });
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        this.source.serialize(writer);
        this.destination.serialize(writer);
        this.frameId.serialize(writer);
        writer.writeBytes(this.reader.readRemaining());
    }

    serializedLength(): number {
        return (
            this.source.serializedLength() +
            this.destination.serializedLength() +
            this.frameId.serializedLength() +
            this.reader.remainingLength()
        );
    }
}

export class ReceivedRoutingFrame extends RoutingFrame {
    previousHop: NodeId;

    constructor(args: RoutingFrameArgs & { previousHop: NodeId }) {
        super(args);
        this.previousHop = args.previousHop;
    }

    static deserializeFromNeighborFrame(frame: NeighborFrame): DeserializeResult<ReceivedRoutingFrame> {
        return RoutingFrame.deserialize(frame.reader).map((routingFrame) => {
            return new ReceivedRoutingFrame({ previousHop: frame.sender.nodeId, ...routingFrame });
        });
    }
}
