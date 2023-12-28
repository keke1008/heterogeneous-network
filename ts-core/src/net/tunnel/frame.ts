import { DeserializeResult } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { TunnelPortId } from "./port";
import { Source } from "../node";
import { RoutingFrame } from "../routing";

interface TunnelFrameArgs {
    sourcePortId: TunnelPortId;
    destinationPortId: TunnelPortId;
    data: BufferReader;
}

export class TunnelFrame {
    sourcePortId: TunnelPortId;
    destinationPortId: TunnelPortId;
    data: BufferReader;

    constructor(args: TunnelFrameArgs) {
        this.sourcePortId = args.sourcePortId;
        this.destinationPortId = args.destinationPortId;
        this.data = args.data;
    }

    static deserialize(reader: BufferReader): DeserializeResult<TunnelFrame> {
        return TunnelPortId.deserialize(reader).andThen((sourcePortId) => {
            return TunnelPortId.deserialize(reader).map((destinationPortId) => {
                return new TunnelFrame({ sourcePortId, destinationPortId, data: reader.subReader() });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        this.sourcePortId.serialize(writer);
        this.destinationPortId.serialize(writer);
        writer.writeBytes(this.data.readRemaining());
    }

    serializedLength(): number {
        return (
            this.sourcePortId.serializedLength() +
            this.destinationPortId.serializedLength() +
            this.data.remainingLength()
        );
    }
}

export class ReceivedTunnelFrame extends TunnelFrame {
    source: Source;

    constructor(args: TunnelFrameArgs & { source: Source }) {
        super(args);
        this.source = args.source;
    }

    static deserializeFromRoutingFrame(frame: RoutingFrame): DeserializeResult<ReceivedTunnelFrame> {
        return TunnelFrame.deserialize(frame.reader).map((tunnelFrame) => {
            return new ReceivedTunnelFrame({ source: frame.source, ...tunnelFrame });
        });
    }
}
