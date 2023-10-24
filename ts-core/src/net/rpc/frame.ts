import { BufferReader, BufferWriter } from "../buffer";

enum FrameType {
    Request = 0x01,
    Response = 0x02,
}

enum Procedure {
    RoutingNeighborSendHello = 0x01,
    RoutingNeighborSendGoodbye = 0x02,
    LinkGetMediaList = 0x03,
    LinkWifiScanAp = 0x04,
    LinkWifiConnectAp = 0x05,
}

enum Result {
    Success = 0x01,
    Failed = 0x02,
    NotSupported = 0x03,
    BadArgument = 0x04,
    Busy = 0x05,
    NotImplemented = 0x06,
}

const numberToEnum = <E>(e: Record<number | string, string | number>) => {
    return (n: number): E => {
        if (n in e) {
            return n as E;
        } else {
            throw new Error(`Unknown enum number ${n}`);
        }
    };
};

const numberToFrameType = numberToEnum<FrameType>(FrameType);
const numberToProcedure = numberToEnum<Procedure>(Procedure);
const numberToResult = numberToEnum<Result>(Result);

export class RequestFrameHeader {
    frameType = FrameType.Request as const;
    procedure: Procedure;

    constructor(procedure: Procedure) {
        this.procedure = procedure;
    }

    static deserialize(reader: BufferReader): RequestFrameHeader {
        const procedure = numberToProcedure(reader.readByte());
        return new RequestFrameHeader(procedure);
    }

    serialize(writer: BufferWriter) {
        writer.writeByte(this.frameType);
        writer.writeByte(this.procedure);
    }

    serializedLength(): number {
        return 1 + 1;
    }
}

export class ResponseFrame {
    frameType = FrameType.Response as const;
    procedure: Procedure;
    result: Result;

    constructor(procedure: Procedure, result: Result) {
        this.procedure = procedure;
        this.result = result;
    }

    static deserialize(reader: BufferReader): ResponseFrame {
        const procedure = numberToProcedure(reader.readByte());
        const result = numberToResult(reader.readByte());
        return new ResponseFrame(procedure, result);
    }

    serialize(writer: BufferWriter) {
        writer.writeByte(this.frameType);
        writer.writeByte(this.procedure);
        writer.writeByte(this.result);
    }

    serializedLength(): number {
        return 1 + 1 + 1;
    }
}

export const deserializeFrame = (reader: BufferReader): RequestFrameHeader | ResponseFrame => {
    const frameType = numberToFrameType(reader.readByte());
    switch (frameType) {
        case FrameType.Request:
            return RequestFrameHeader.deserialize(reader);
        case FrameType.Response:
            return ResponseFrame.deserialize(reader);
        default:
            throw new Error(`Unknown frame type ${frameType}`);
    }
};

export const serializeFrame = (frame: RequestFrameHeader | ResponseFrame): BufferReader => {
    const length = frame.serializedLength();
    const writer = new BufferWriter(new Uint8Array(length));
    frame.serialize(writer);
    return new BufferReader(writer.unwrapBuffer());
};
