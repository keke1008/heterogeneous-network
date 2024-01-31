import { BooleanSerdeable, ObjectSerdeable, RemainingBytesSerdeable, TransformSerdeable } from "@core/serde";

export class StreamFrame {
    fin: boolean;
    data: Uint8Array;

    constructor(args: { fin: boolean; data: Uint8Array }) {
        this.fin = args.fin;
        this.data = args.data;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            fin: new BooleanSerdeable(),
            data: new RemainingBytesSerdeable(),
        }),
        (obj) => new StreamFrame(obj),
        (frame) => frame,
    );

    static headerLength(): number {
        return StreamFrame.serdeable
            .serializer(new StreamFrame({ fin: false, data: new Uint8Array() }))
            .serializedLength();
    }
}
