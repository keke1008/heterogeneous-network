import { BufferReader, BufferWriter } from "../buffer";
import { RoutingFrame } from "../routing";
import { NodeId } from "../node";
import { DeserializeResult, InvalidValueError } from "@core/serde";
import { Err, Ok } from "oxide.ts";
import { RequestId } from "./requestId";

export enum FrameType {
    Request = 1,
    Response = 2,
}

export enum Procedure {
    // Debug 1~99
    Blink = 1,

    // Media 100~199
    GetMediaList = 100,

    // Uhf 200~299

    // Wifi 300~399
    ScanAccessPoints = 300,
    GetConnectedAccessPoint = 301,
    ConnectToAccessPoint = 302,
    DisconnectFromAccessPoint = 303,
    StartServer = 320,

    // Serial 400~499

    // Neighbor 500~599
    SendHello = 500,
    SendGoodbye = 501,
    GetNeighborList = 520,
}

export enum RpcStatus {
    Success = 1,
    Failed = 2,
    NotSupported = 3,
    BadArgument = 4,
    Busy = 5,
    NotImplemented = 6,
    Timeout = 7,
    Unreachable = 8,
    BadResponseFormat = 9,
}

const FRAME_TYPE_LENGTH = 1;
const PROCEDURE_LENGTH = 2;
const RPC_STATUS_LENGTH = 1;

const deserializeFrameType = (reader: BufferReader): DeserializeResult<FrameType> => {
    const frameType = reader.readByte();
    return frameType in FrameType ? Ok(frameType) : Err(new InvalidValueError(`Invalid frame type ${frameType}`));
};

const deserializeProcedure = (reader: BufferReader): DeserializeResult<Procedure> => {
    const procedure = reader.readUint16();
    return procedure in Procedure ? Ok(procedure) : Err(new InvalidValueError(`Invalid procedure ${procedure}`));
};

const deserializeRpcStatus = (reader: BufferReader): DeserializeResult<RpcStatus> => {
    const status = reader.readByte();
    return status in RpcStatus ? Ok(status) : Err(new InvalidValueError(`Invalid status ${status}`));
};

class RequestFrameHeader {
    frameType = FrameType.Request as const;
    procedure: Procedure;
    requestId: RequestId;

    constructor(opts: { procedure: Procedure; requestId: RequestId }) {
        this.procedure = opts.procedure;
        this.requestId = opts.requestId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<RequestFrameHeader> {
        return deserializeProcedure(reader).andThen((procedure) => {
            return RequestId.deserialize(reader).map((requestId) => {
                return new RequestFrameHeader({ procedure, requestId });
            });
        });
    }

    serialize(writer: BufferWriter) {
        writer.writeByte(this.frameType);
        writer.writeUint16(this.procedure);
        this.requestId.serialize(writer);
    }

    serializedLength(): number {
        return FRAME_TYPE_LENGTH + PROCEDURE_LENGTH + this.requestId.serializedLength();
    }
}

class ResponseFrameHeader {
    frameType = FrameType.Response as const;
    procedure: Procedure;
    requestId: RequestId;
    status: RpcStatus;

    constructor(opts: { procedure: Procedure; requestId: RequestId; status: RpcStatus }) {
        this.procedure = opts.procedure;
        this.requestId = opts.requestId;
        this.status = opts.status;
    }

    static deserialize(reader: BufferReader): DeserializeResult<ResponseFrameHeader> {
        return deserializeProcedure(reader).andThen((procedure) => {
            return RequestId.deserialize(reader).andThen((requestId) => {
                return deserializeRpcStatus(reader).map((status) => {
                    return new ResponseFrameHeader({ procedure, requestId, status });
                });
            });
        });
    }

    serialize(writer: BufferWriter) {
        writer.writeByte(this.frameType);
        writer.writeUint16(this.procedure);
        this.requestId.serialize(writer);
        writer.writeByte(this.status);
    }

    serializedLength(): number {
        return FRAME_TYPE_LENGTH + PROCEDURE_LENGTH + this.requestId.serializedLength() + RPC_STATUS_LENGTH;
    }
}

const deserializeFrameHeader = (reader: BufferReader): DeserializeResult<RequestFrameHeader | ResponseFrameHeader> => {
    return deserializeFrameType(reader).andThen(
        (frameType): DeserializeResult<RequestFrameHeader | ResponseFrameHeader> => {
            switch (frameType) {
                case FrameType.Request:
                    return RequestFrameHeader.deserialize(reader);
                case FrameType.Response:
                    return ResponseFrameHeader.deserialize(reader);
            }
        },
    );
};

export interface RpcRequest {
    frameType: FrameType.Request;
    procedure: Procedure;
    requestId: RequestId;
    senderId: NodeId;
    targetId: NodeId;
    bodyReader: BufferReader;
}

export interface RpcResponse {
    frameType: FrameType.Response;
    procedure: Procedure;
    requestId: RequestId;
    status: RpcStatus;
    senderId: NodeId;
    targetId: NodeId;
    bodyReader: BufferReader;
}

export const createResponse = (
    request: RpcRequest,
    args: { status: RpcStatus; reader?: BufferReader },
): RpcResponse => {
    const bodyReader = args.reader ?? new BufferReader(new Uint8Array(0));
    return {
        frameType: FrameType.Response,
        procedure: request.procedure,
        requestId: request.requestId,
        status: args.status,
        senderId: request.targetId,
        targetId: request.senderId,
        bodyReader,
    };
};

export const deserializeFrame = (frame: RoutingFrame): DeserializeResult<RpcRequest | RpcResponse> => {
    return deserializeFrameHeader(frame.reader).map((header) => {
        if (header.frameType === FrameType.Request) {
            return {
                frameType: header.frameType,
                procedure: header.procedure,
                requestId: header.requestId,
                senderId: frame.header.senderId,
                targetId: frame.header.targetId,
                bodyReader: frame.reader,
            };
        } else {
            return {
                frameType: header.frameType,
                procedure: header.procedure,
                requestId: header.requestId,
                status: header.status,
                senderId: frame.header.senderId,
                targetId: frame.header.targetId,
                bodyReader: frame.reader,
            };
        }
    });
};

export const serializeFrame = (frame: RpcRequest | RpcResponse): BufferReader => {
    const header =
        frame.frameType === FrameType.Request
            ? new RequestFrameHeader({ procedure: frame.procedure, requestId: frame.requestId })
            : new ResponseFrameHeader({ procedure: frame.procedure, requestId: frame.requestId, status: frame.status });

    const length = header.serializedLength() + frame.bodyReader.readableLength();
    const writer = new BufferWriter(new Uint8Array(length));
    header.serialize(writer);
    writer.writeBytes(frame.bodyReader.readRemaining());

    return new BufferReader(writer.unwrapBuffer());
};
