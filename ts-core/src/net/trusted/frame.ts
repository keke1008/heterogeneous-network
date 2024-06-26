// Checksumフィールドは16ビットなので，フレームを16ビット間隔で区切った際に
// Checksumフィールドが2つに分割されていると，正しくChecksumが計算できなくて死ぬ．
// なので気を付けること．

import {
    ConstantSerdeable,
    DeserializeResult,
    ObjectSerdeable,
    RemainingBytesSerdeable,
    TransformSerdeable,
    Uint8Serdeable,
    VariantSerdeable,
} from "@core/serde";
import { Checksum, ChecksumWriter } from "./checksum";
import { Destination, Source } from "../node";
import { ReceivedTunnelFrame, TunnelPortId } from "../tunnel";
import { BufferReader } from "../buffer";
import { AcknowledgementNumber, SequenceNumber } from "./sequenceNumber";

export interface LengthOmittedPseudoHeader {
    source: Source;
    destination: Destination;
    sourcePort: TunnelPortId;
    destinationPortId: TunnelPortId;
}

export class PseudoHeader {
    source: Source;
    destination: Destination;
    sourcePort: TunnelPortId;
    destinationPortId: TunnelPortId;
    length: number;

    constructor(args: {
        source: Source;
        destination: Destination;
        sourcePort: TunnelPortId;
        destinationPortId: TunnelPortId;
        length: number;
    }) {
        this.source = args.source;
        this.destination = args.destination;
        this.sourcePort = args.sourcePort;
        this.destinationPortId = args.destinationPortId;
        this.length = args.length;
    }

    static readonly serdeable = new ObjectSerdeable({
        source: Source.serdeable, // 8 bytes
        destination: Destination.serdeable, // 8 bytes
        sourcePort: TunnelPortId.serdeable, // 2 bytes
        destinationPortId: TunnelPortId.serdeable, // 2 bytes
        length: new Uint8Serdeable(), // 1 byte
    });

    static fromReceivedTunnelFrame(frame: ReceivedTunnelFrame): PseudoHeader {
        return new PseudoHeader({
            source: frame.source,
            destination: frame.destination,
            sourcePort: frame.sourcePortId,
            destinationPortId: frame.destinationPortId,
            length: frame.data.length,
        });
    }

    createResponse(args: { source: Source }): PseudoHeader {
        return new PseudoHeader({
            source: args.source,
            destination: this.source.intoDestination(),
            sourcePort: this.destinationPortId,
            destinationPortId: this.sourcePort,
            length: this.length,
        });
    }
}

export enum FrameType {
    Syn = 1,
    SynAck = 2,
    Fin = 3,
    FinAck = 4,
    Data = 5,
    DataAck = 6,
}

export class SynFrameBody {
    readonly frameType = FrameType.Syn as const;
    static readonly serdeable = new ConstantSerdeable(new SynFrameBody());
    isCorrespondingAckFrame(body: TrustedFrameBody): boolean {
        return body.frameType === FrameType.SynAck;
    }
}

export class SynAckFrameBody {
    readonly frameType = FrameType.SynAck as const;
    static readonly serdeable = new ConstantSerdeable(new SynAckFrameBody());
}

export class FinFrameBody {
    readonly frameType = FrameType.Fin as const;
    static readonly serdeable = new ConstantSerdeable(new FinFrameBody());
    isCorrespondingAckFrame(body: TrustedFrameBody): boolean {
        return body.frameType === FrameType.FinAck;
    }
}

export class FinAckFrameBody {
    readonly frameType = FrameType.FinAck as const;
    static readonly serdeable = new ConstantSerdeable(new FinAckFrameBody());
}

export class DataFrameBody {
    readonly frameType = FrameType.Data;
    sequenceNumber: SequenceNumber;
    acknowledgementNumber: AcknowledgementNumber;
    payload: Uint8Array;

    constructor(args: {
        sequenceNumber: SequenceNumber;
        acknowledgementNumber: AcknowledgementNumber;
        payload: Uint8Array;
    }) {
        this.sequenceNumber = args.sequenceNumber;
        this.acknowledgementNumber = args.acknowledgementNumber;
        this.payload = args.payload;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            sequenceNumber: SequenceNumber.serdeable,
            acknowledgementNumber: AcknowledgementNumber.serdeable,
            payload: new RemainingBytesSerdeable(),
        }),
        (obj) => new DataFrameBody(obj),
        (frame) => frame,
    );

    static headerLength(): number {
        return DataFrameBody.serdeable
            .serializer(
                new DataFrameBody({
                    sequenceNumber: new SequenceNumber(0),
                    acknowledgementNumber: new AcknowledgementNumber(0),
                    payload: new Uint8Array(),
                }),
            )
            .serializedLength();
    }
}

export class DataAckFrameBody {
    readonly frameType = FrameType.DataAck as const;
    acknowledgementNumber: AcknowledgementNumber;

    constructor(acknowledgementNumber: AcknowledgementNumber) {
        this.acknowledgementNumber = acknowledgementNumber;
    }

    static readonly serdeable = new TransformSerdeable(
        AcknowledgementNumber.serdeable,
        (value) => new DataAckFrameBody(value),
        (frame) => frame.acknowledgementNumber,
    );
}

export type TrustedFrameBody =
    | SynFrameBody
    | SynAckFrameBody
    | FinFrameBody
    | FinAckFrameBody
    | DataFrameBody
    | DataAckFrameBody;
export type TrustedRequestFrameBody = SynFrameBody | FinFrameBody | DataFrameBody;
export type TrustedAckFrameBody = Exclude<TrustedFrameBody, TrustedRequestFrameBody>;

export class TrustedFrame {
    checksum: Checksum;
    body: TrustedFrameBody;

    protected constructor(args: { checksum: Checksum; body: TrustedFrameBody }) {
        this.checksum = args.checksum;
        this.body = args.body;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            checksum: Checksum.serdeable,
            body: new VariantSerdeable(
                [SynFrameBody, SynAckFrameBody, FinFrameBody, FinAckFrameBody, DataFrameBody, DataAckFrameBody].map(
                    (body) => body.serdeable,
                ),
                (frame) => frame.frameType,
            ),
        }),
        (value) => new TrustedFrame(value),
        (frame) => frame,
    );

    static create<Body extends TrustedFrameBody>(args: {
        body: Body;
        pseudoHeader: LengthOmittedPseudoHeader;
    }): TrustedFrame & { body: Body } {
        const frame = new TrustedFrame({
            checksum: Checksum.zero(),
            body: args.body,
        }) as TrustedFrame & { body: Body };
        const serializer = TrustedFrame.serdeable.serializer(frame);
        const pseudoHeaderSerializer = PseudoHeader.serdeable.serializer({
            length: serializer.serializedLength(),
            ...args.pseudoHeader,
        });

        const writer = new ChecksumWriter();
        serializer.serialize(writer);
        pseudoHeaderSerializer.serialize(writer);
        frame.checksum = Checksum.fromWriter(writer);

        return frame;
    }

    static headerLength(): number {
        return TrustedFrame.serdeable
            .serializer(new TrustedFrame({ checksum: Checksum.zero(), body: new SynFrameBody() }))
            .serializedLength();
    }
}

export type TrustedDataFrame = TrustedFrame & { body: DataFrameBody };
export const TrustedDataFrame = {
    headerLength(): number {
        return TrustedFrame.headerLength() + DataFrameBody.headerLength();
    },
};

export type TrustedRequestFrame = TrustedFrame & { body: TrustedRequestFrameBody };

export class ReceivedTrustedFrame extends TrustedFrame {
    pseudoHeader: PseudoHeader;

    private constructor(args: { frame: TrustedFrame; pseudoHeader: PseudoHeader }) {
        super(args.frame);
        this.pseudoHeader = args.pseudoHeader;
    }

    static deserialize(tunnelFrame: ReceivedTunnelFrame): DeserializeResult<ReceivedTrustedFrame> {
        return BufferReader.deserialize(TrustedFrame.serdeable.deserializer(), tunnelFrame.data).map((frame) => {
            const pseudoHeader = PseudoHeader.fromReceivedTunnelFrame(tunnelFrame);
            return new ReceivedTrustedFrame({ frame, pseudoHeader });
        });
    }

    verifyChecksum(): boolean {
        const serializer = TrustedFrame.serdeable.serializer(this);
        const pseudoHeaderSerializer = PseudoHeader.serdeable.serializer(this.pseudoHeader);

        const writer = new ChecksumWriter();
        serializer.serialize(writer);
        pseudoHeaderSerializer.serialize(writer);
        return writer.verify();
    }
}
