import { Source } from "@core/net/node";
import { RemainingBytesSerdeable, TransformSerdeable, TupleSerdeable } from "@core/serde";

export class NeighborFrame {
    sender: Source;
    payload: Uint8Array;

    constructor(args: { sender: Source; payload: Uint8Array }) {
        this.sender = args.sender;
        this.payload = args.payload;
    }

    static readonly serdeable = new TransformSerdeable(
        new TupleSerdeable([Source.serdeable, new RemainingBytesSerdeable()] as const),
        ([sender, payload]) => new NeighborFrame({ sender, payload }),
        (frame) => [frame.sender, frame.payload] as const,
    );
}
