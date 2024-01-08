// Checksumフィールドは16ビットなので，フレームを16ビット間隔で区切った際に
// Checksumフィールドが2つに分割されていると，正しくChecksumが計算できなくて死ぬ．
// なので気を付けること．

import {
    ConstantSerdeable,
    DeserializeResult,
    ObjectSerdeable,
    RemainingBytesSerdeable,
    TransformSerdeable,
    TupleSerdeable,
    Uint8Serdeable,
    VariantSerdeable,
} from "@core/serde";
import { Checksum, ChecksumWriter } from "./checksum";
import { Destination, Source } from "../node";
import { ReceivedTunnelFrame, TunnelPortId } from "../tunnel";
import { BufferReader } from "../buffer";
import { SequenceNumber } from "./sequenceNumber";

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
    payload: Uint8Array;

    constructor(sequenceNumber: SequenceNumber, payload: Uint8Array) {
        this.sequenceNumber = sequenceNumber;
        this.payload = payload;
    }

    static readonly serdeable = new TransformSerdeable(
        new TupleSerdeable([SequenceNumber.serdeable, new RemainingBytesSerdeable()] as const),
        (value) => new DataFrameBody(...value),
        (frame) => [frame.sequenceNumber, frame.payload] as const,
    );

    isCorrespondingAckFrame(body: TrustedFrameBody): boolean {
        return body.frameType === FrameType.DataAck && body.sequenceNumber.equals(this.sequenceNumber);
    }

    createAckFrameBody(): DataAckFrameBody {
        return new DataAckFrameBody(this.sequenceNumber);
    }
}

export class DataAckFrameBody {
    readonly frameType = FrameType.DataAck as const;
    sequenceNumber: SequenceNumber;

    constructor(sequenceNumber: SequenceNumber) {
        this.sequenceNumber = sequenceNumber;
    }

    static readonly serdeable = new TransformSerdeable(
        SequenceNumber.serdeable,
        (value) => new DataAckFrameBody(value),
        (frame) => frame.sequenceNumber,
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
        pseudoHeaderSerializer.serialize(writer);
        serializer.serialize(writer);
        frame.checksum = Checksum.fromWriter(writer);

        return frame;
    }
}

export type TrustedDataFrame = TrustedFrame & { body: DataFrameBody };
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
        pseudoHeaderSerializer.serialize(writer);
        serializer.serialize(writer);
        return writer.verify();
    }
}
