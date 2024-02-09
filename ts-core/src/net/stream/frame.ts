import { ObjectSerdeable, RemainingBytesSerdeable, TransformSerdeable, Uint32Serdeable } from "@core/serde";
import { P, match } from "ts-pattern";
import { BufferWriter } from "../buffer";

export class StreamHeadFrame {
    totalByteLength: number;
    body: Uint8Array;

    constructor(args: { totalByteLength: number; body: Uint8Array }) {
        this.totalByteLength = args.totalByteLength;
        this.body = args.body;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            totalByteLength: new Uint32Serdeable(),
            body: new RemainingBytesSerdeable(),
        }),
        (obj) => new StreamHeadFrame(obj),
        (header) => header,
    );

    static headerLength(): number {
        return StreamHeadFrame.serdeable
            .serializer(new StreamHeadFrame({ totalByteLength: 0, body: new Uint8Array() }))
            .serializedLength();
    }

    sendDataLength(): number {
        return this.body.length;
    }
}

export class StreamBodyFrame {
    body: Uint8Array;

    constructor(args: { body: Uint8Array }) {
        this.body = args.body;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ body: new RemainingBytesSerdeable() }),
        (obj) => new StreamBodyFrame(obj),
        (header) => header,
    );

    sendDataLength(): number {
        return this.body.length;
    }
}

export type StreamFrame = StreamHeadFrame | StreamBodyFrame;
export const StreamFrame = {
    fromData(data: Uint8Array, mtu: number): StreamFrame[] {
        const bodyInHeadLength = mtu - StreamHeadFrame.headerLength();
        const head = new StreamHeadFrame({
            totalByteLength: data.length,
            body: data.subarray(0, bodyInHeadLength),
        });

        const remaining = data.subarray(bodyInHeadLength);

        let offset = 0;
        const frames: StreamFrame[] = [head];
        while (offset < remaining.length) {
            const body = remaining.subarray(offset, offset + mtu);
            frames.push(new StreamBodyFrame({ body }));
            offset += mtu;
        }

        return frames;
    },

    serialize(frame: StreamFrame): Uint8Array {
        const serializer = match(frame)
            .with(P.instanceOf(StreamHeadFrame), (head) => StreamHeadFrame.serdeable.serializer(head))
            .with(P.instanceOf(StreamBodyFrame), (body) => StreamBodyFrame.serdeable.serializer(body))
            .exhaustive();
        return BufferWriter.serialize(serializer).unwrap();
    },
};
