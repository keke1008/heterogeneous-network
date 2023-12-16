import { DeserializeResult } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { Destination, FrameId } from "../discovery";
import { NodeId } from "../node";

export class RoutingFrame {
    sourceId: NodeId;
    destination: Destination;
    frameId: FrameId;
    reader: BufferReader;

    constructor(opts: { sourceId: NodeId; destination: Destination; frameId: FrameId; reader: BufferReader }) {
        this.sourceId = opts.sourceId;
        this.destination = opts.destination;
        this.frameId = opts.frameId;
        this.reader = opts.reader;
    }

    static deserialize(reader: BufferReader): DeserializeResult<RoutingFrame> {
        return NodeId.deserialize(reader).andThen((sourceId) => {
            return Destination.deserialize(reader).andThen((destination) => {
                return FrameId.deserialize(reader).map((frameId) => {
                    return new RoutingFrame({ sourceId, destination, frameId, reader });
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        this.sourceId.serialize(writer);
        this.destination.serialize(writer);
        this.frameId.serialize(writer);
        writer.writeBytes(this.reader.readRemaining());
    }

    serializedLength(): number {
        return (
            this.sourceId.serializedLength() +
            this.destination.serializedLength() +
            this.frameId.serializedLength() +
            this.reader.remainingLength()
        );
    }
}
