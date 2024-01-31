import { ObjectSerdeable, RemainingBytesSerdeable, TransformSerdeable, Uint8Serdeable } from "@core/serde";
import { BitflagsSerdeable } from "@core/serde/bitflags";

export enum StreamFlags {
    None = 0b0000,
    Fin = 0b0001,
    Cancel = 0b0010,
}

export class StreamFrame {
    flags: StreamFlags;
    data: Uint8Array;

    constructor(args: { flags: StreamFlags; data: Uint8Array }) {
        this.flags = args.flags;
        this.data = args.data;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            flags: new BitflagsSerdeable<StreamFlags>(StreamFlags, new Uint8Serdeable()),
            data: new RemainingBytesSerdeable(),
        }),
        (obj) => new StreamFrame(obj),
        (frame) => frame,
    );

    static headerLength(): number {
        return StreamFrame.serdeable
            .serializer(new StreamFrame({ flags: StreamFlags.None, data: new Uint8Array() }))
            .serializedLength();
    }
}
