import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Source } from "@core/net/node";
import { DeserializeResult } from "@core/serde";

export class NeighborFrame {
    sender: Source;
    reader: BufferReader;

    constructor(args: { sender: Source; reader: BufferReader }) {
        this.sender = args.sender;
        this.reader = args.reader;
    }

    static deserialize(reader: BufferReader): DeserializeResult<NeighborFrame> {
        return Source.deserialize(reader).map((sender) => {
            return new NeighborFrame({ sender, reader: reader.subReader() });
        });
    }

    serialize(writer: BufferWriter): void {
        this.sender.serialize(writer);
        writer.writeBytes(this.reader.readRemaining());
    }

    serializedLength(): number {
        return this.sender.serializedLength() + this.reader.remainingLength();
    }
}
