import { BufferReader, BufferWriter } from "../buffer";
import { RoutingFrame } from "../routing";
import { NodeId } from "../node";
import { FrameId } from "../link";
import { DeserializeResult, InvalidValueError } from "@core/serde";
import { Err, Ok } from "oxide.ts";

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
    frameId: FrameId;

    constructor(opts: { procedure: Procedure; frameId: FrameId }) {
        this.procedure = opts.procedure;
        this.frameId = opts.frameId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<RequestFrameHeader> {
        return deserializeProcedure(reader).andThen((procedure) => {
            return FrameId.deserialize(reader).map((frameId) => {
                return new RequestFrameHeader({ procedure, frameId });
            });
        });
    }

    serialize(writer: BufferWriter) {
        writer.writeByte(this.frameType);
        writer.writeUint16(this.procedure);
        this.frameId.serialize(writer);
    }

    serializedLength(): number {
        return FRAME_TYPE_LENGTH + PROCEDURE_LENGTH + this.frameId.serializedLength();
    }
}

class ResponseFrameHeader {
    frameType = FrameType.Response as const;
    procedure: Procedure;
    frameId: FrameId;
    status: RpcStatus;

    constructor(opts: { procedure: Procedure; frameId: FrameId; status: RpcStatus }) {
        this.procedure = opts.procedure;
        this.frameId = opts.frameId;
        this.status = opts.status;
    }

    static deserialize(reader: BufferReader): DeserializeResult<ResponseFrameHeader> {
        return deserializeProcedure(reader).andThen((procedure) => {
            return FrameId.deserialize(reader).andThen((frameId) => {
                return deserializeRpcStatus(reader).map((status) => {
                    return new ResponseFrameHeader({ procedure, frameId, status });
                });
            });
        });
    }

    serialize(writer: BufferWriter) {
        writer.writeByte(this.frameType);
        writer.writeUint16(this.procedure);
        writer.writeByte(this.status);
    }

    serializedLength(): number {
        return FRAME_TYPE_LENGTH + PROCEDURE_LENGTH + this.frameId.serializedLength() + RPC_STATUS_LENGTH;
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
    frameId: FrameId;
    senderId: NodeId;
    targetId: NodeId;
    bodyReader: BufferReader;
}

export interface RpcResponse {
    frameType: FrameType.Response;
    procedure: Procedure;
    frameId: FrameId;
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
        frameId: request.frameId,
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
                frameId: header.frameId,
                senderId: frame.senderId,
                targetId: frame.targetId,
                bodyReader: frame.reader,
            };
        } else {
            return {
                frameType: header.frameType,
                procedure: header.procedure,
                frameId: header.frameId,
                status: header.status,
                senderId: frame.senderId,
                targetId: frame.targetId,
                bodyReader: frame.reader,
            };
        }
    });
};

export const serializeFrame = (frame: RpcRequest | RpcResponse): BufferReader => {
    const header =
        frame.frameType === FrameType.Request
            ? new RequestFrameHeader({ procedure: frame.procedure, frameId: frame.frameId })
            : new ResponseFrameHeader({ procedure: frame.procedure, frameId: frame.frameId, status: frame.status });

    const length = header.serializedLength() + frame.bodyReader.readableLength();
    const writer = new BufferWriter(new Uint8Array(length));
    header.serialize(writer);
    writer.writeBytes(frame.bodyReader.readRemaining());

    return new BufferReader(writer.unwrapBuffer());
};
