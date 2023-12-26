import { DeserializeResult } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { FrameId } from "../discovery";
import { Source, Destination, NodeId } from "../node";

export class RoutingFrame {
    source: Source;
    destination: Destination;
    previousHop: NodeId;
    frameId: FrameId;
    reader: BufferReader;

    constructor(opts: {
        source: Source;
        destination: Destination;
        previousHop: NodeId;
        frameId: FrameId;
        reader: BufferReader;
    }) {
        this.source = opts.source;
        this.destination = opts.destination;
        this.previousHop = opts.previousHop;
        this.frameId = opts.frameId;
        this.reader = opts.reader;
    }

    static deserialize(reader: BufferReader): DeserializeResult<RoutingFrame> {
        return Source.deserialize(reader).andThen((source) => {
            return Destination.deserialize(reader).andThen((destination) => {
                return NodeId.deserialize(reader).andThen((previousHop) => {
                    return FrameId.deserialize(reader).map((frameId) => {
                        return new RoutingFrame({ source, destination, previousHop, frameId, reader });
                    });
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        this.source.serialize(writer);
        this.destination.serialize(writer);
        this.previousHop.serialize(writer);
        this.frameId.serialize(writer);
        writer.writeBytes(this.reader.readRemaining());
    }

    serializedLength(): number {
        return (
            this.source.serializedLength() +
            this.destination.serializedLength() +
            this.previousHop.serializedLength() +
            this.frameId.serializedLength() +
            this.reader.remainingLength()
        );
    }
}
