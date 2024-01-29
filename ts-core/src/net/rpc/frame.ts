import { BufferReader, BufferWriter } from "../buffer";
import { RoutingFrame } from "../routing";
import {
    DeserializeResult,
    EnumSerdeable,
    RemainingBytesSerdeable,
    TransformSerdeable,
    TupleSerdeable,
    Uint16Serdeable,
    VariantSerdeable,
} from "@core/serde";
import { Ok } from "oxide.ts";
import { RequestId } from "./requestId";
import { Source, Destination } from "../node";

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
    CloseServer = 321,

    // Serial 400~499
    SetAddress = 400,

    // Ethernet 500~599
    SetEthernetIpAddress = 500,
    SetEthernetSubnetMask = 501,

    // Notification 2000~2099

    // Local 2100~2199
    SetCost = 2100,
    SetClusterId = 2101,

    // Neighbor 2200~2299
    SendHello = 2200,
    GetNeighborList = 2220,

    // Discovery 2300~2399

    // Routing 2400~2499

    // Address 4000~4099
    ResolveAddress = 4000,

    // VRouter 5000~5099
    GetVRouters = 5000,
    CreateVRouter = 5001,
    DeleteVRouter = 5002,
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
    InvalidOperation = 10,
}

const ProcedureSerdeable = new EnumSerdeable<Procedure>(Procedure, new Uint16Serdeable());
const RpcStatusSerdeable = new EnumSerdeable<RpcStatus>(RpcStatus);

class RequestFrame {
    frameType = FrameType.Request as const;
    procedure: Procedure;
    requestId: RequestId;
    body: Uint8Array;

    constructor(opts: { procedure: Procedure; requestId: RequestId; body: Uint8Array }) {
        this.procedure = opts.procedure;
        this.requestId = opts.requestId;
        this.body = opts.body;
    }

    static readonly serdeable = new TransformSerdeable(
        new TupleSerdeable([ProcedureSerdeable, RequestId.serdeable, new RemainingBytesSerdeable()] as const),
        ([procedure, requestId, body]) => new RequestFrame({ procedure, requestId, body }),
        (header) => [header.procedure, header.requestId, header.body] as const,
    );
}

class ResponseFrame {
    frameType = FrameType.Response as const;
    procedure: Procedure;
    requestId: RequestId;
    status: RpcStatus;
    body: Uint8Array;

    constructor(opts: { procedure: Procedure; requestId: RequestId; status: RpcStatus; body: Uint8Array }) {
        this.procedure = opts.procedure;
        this.requestId = opts.requestId;
        this.status = opts.status;
        this.body = opts.body;
    }

    static readonly serdeable = new TransformSerdeable(
        new TupleSerdeable([
            ProcedureSerdeable,
            RequestId.serdeable,
            RpcStatusSerdeable,
            new RemainingBytesSerdeable(),
        ] as const),
        ([procedure, requestId, status, body]) => new ResponseFrame({ procedure, requestId, status, body }),
        (header) => [header.procedure, header.requestId, header.status, header.body] as const,
    );
}

const RpcFrame = {
    serdeable: new VariantSerdeable(
        [RequestFrame.serdeable, ResponseFrame.serdeable] as const,
        (frame) => frame.frameType,
    ),
};

export interface RpcRequest {
    frameType: FrameType.Request;
    procedure: Procedure;
    requestId: RequestId;
    client: Source;
    server: Destination;
    bodyReader: BufferReader;
}

export interface RpcResponse {
    frameType: FrameType.Response;
    procedure: Procedure;
    requestId: RequestId;
    status: RpcStatus;
    bodyReader: BufferReader;
}

export const deserializeFrame = (routingFrame: RoutingFrame): DeserializeResult<RpcRequest | RpcResponse> => {
    const reader = new BufferReader(routingFrame.payload);
    const result = RpcFrame.serdeable.deserializer().deserialize(reader);
    if (result.isErr()) {
        return result;
    }

    const frame = result.unwrap();
    if (frame.frameType === FrameType.Request) {
        return Ok({
            frameType: frame.frameType,
            procedure: frame.procedure,
            requestId: frame.requestId,
            client: routingFrame.source,
            server: routingFrame.destination,
            bodyReader: new BufferReader(frame.body),
        } satisfies RpcRequest);
    } else {
        const destinationId = routingFrame.destination.nodeId;
        if (destinationId === undefined) {
            console.warn("Received rpc response frame with broadcast destination", routingFrame);
            reader.invalidValueError("destination", "Received rpc response frame with broadcast destination");
        }
        return Ok({
            frameType: frame.frameType,
            procedure: frame.procedure,
            requestId: frame.requestId,
            status: frame.status,
            bodyReader: new BufferReader(frame.body),
        } satisfies RpcResponse);
    }
};

export const serializeFrame = (value: RpcRequest | RpcResponse): Uint8Array => {
    const body = value.bodyReader.readRemaining();
    const frame =
        value.frameType === FrameType.Request
            ? new RequestFrame({ procedure: value.procedure, requestId: value.requestId, body })
            : new ResponseFrame({ procedure: value.procedure, requestId: value.requestId, status: value.status, body });

    return BufferWriter.serialize(RpcFrame.serdeable.serializer(frame)).expect("Failed to serialize frame");
};
