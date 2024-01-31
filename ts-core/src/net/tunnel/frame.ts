import { DeserializeResult, ObjectSerdeable, RemainingBytesSerdeable } from "@core/serde";
import { BufferReader } from "../buffer";
import { TunnelPortId } from "./port";
import { Destination, Source } from "../node";
import { RoutingFrame } from "../routing";

interface TunnelFrameArgs {
    sourcePortId: TunnelPortId;
    destinationPortId: TunnelPortId;
    data: Uint8Array;
}

export class TunnelFrame {
    sourcePortId: TunnelPortId;
    destinationPortId: TunnelPortId;
    data: Uint8Array;

    constructor(args: TunnelFrameArgs) {
        this.sourcePortId = args.sourcePortId;
        this.destinationPortId = args.destinationPortId;
        this.data = args.data;
    }

    static readonly serdeable = new ObjectSerdeable({
        sourcePortId: TunnelPortId.serdeable,
        destinationPortId: TunnelPortId.serdeable,
        data: new RemainingBytesSerdeable(),
    });

    static headerLength(): number {
        return TunnelPortId.serializedLength() * 2;
    }
}

export class ReceivedTunnelFrame extends TunnelFrame {
    source: Source;
    destination: Destination;

    constructor(args: TunnelFrameArgs & { source: Source; destination: Destination }) {
        super(args);
        this.source = args.source;
        this.destination = args.destination;
    }

    static deserializeFromRoutingFrame(frame: RoutingFrame): DeserializeResult<ReceivedTunnelFrame> {
        return TunnelFrame.serdeable
            .deserializer()
            .deserialize(new BufferReader(frame.payload))
            .map((tunnelFrame) => {
                return new ReceivedTunnelFrame({
                    source: frame.source,
                    destination: frame.destination,
                    ...tunnelFrame,
                });
            });
    }
}
