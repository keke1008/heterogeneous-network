import { DeserializeResult, ObjectSerdeable, RemainingBytesSerdeable } from "@core/serde";
import { BufferReader } from "../buffer";
import { TunnelPortId } from "./port";
import { Source } from "../node";
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
}

export class ReceivedTunnelFrame extends TunnelFrame {
    source: Source;

    constructor(args: TunnelFrameArgs & { source: Source }) {
        super(args);
        this.source = args.source;
    }

    static deserializeFromRoutingFrame(frame: RoutingFrame): DeserializeResult<ReceivedTunnelFrame> {
        return TunnelFrame.serdeable
            .deserializer()
            .deserialize(new BufferReader(frame.payload))
            .map((tunnelFrame) => {
                return new ReceivedTunnelFrame({ source: frame.source, ...tunnelFrame });
            });
    }
}
