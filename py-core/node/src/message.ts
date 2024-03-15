import { Address, Cost, NodeId, TunnelPortId } from "@core/net";
import {
    EmptySerdeable,
    EnumSerdeable,
    ObjectSerdeable,
    RemainingBytesSerdeable,
    TransformSerdeable,
    Uint32Serdeable,
    Utf8Serdeable,
    VariantSerdeable,
} from "@core/serde";

export class MessageDescriptor {
    descriptor: number;

    constructor(descriptor: number) {
        this.descriptor = descriptor;
    }

    static readonly serdeable = new TransformSerdeable(
        new Uint32Serdeable(),
        (descriptor) => new MessageDescriptor(descriptor),
        (obj) => obj.descriptor,
    );
}

export class SendHello {
    index = 1;
    descriptor: MessageDescriptor;
    address: Address;
    cost: Cost;

    constructor(args: { descriptor: MessageDescriptor; address: Address; cost: Cost }) {
        this.descriptor = args.descriptor;
        this.address = args.address;
        this.cost = args.cost;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            descriptor: MessageDescriptor.serdeable,
            address: Address.serdeable,
            cost: Cost.serdeable,
        }),
        (obj) => new SendHello(obj),
        (obj) => obj,
    );
}

export class ServerDescriptor {
    descriptor: number;

    constructor(descriptor: number) {
        this.descriptor = descriptor;
    }

    get value(): number {
        return this.descriptor;
    }

    static readonly serdeable = new TransformSerdeable(
        new Uint32Serdeable(),
        (obj) => new ServerDescriptor(obj),
        (obj) => obj.descriptor,
    );
}

export class SocketDescriptor {
    descriptor: number;

    constructor(descriptor: number) {
        this.descriptor = descriptor;
    }

    get value(): number {
        return this.descriptor;
    }

    static readonly serdeable = new TransformSerdeable(
        new Uint32Serdeable(),
        (obj) => new SocketDescriptor(obj),
        (obj) => obj.descriptor,
    );
}

export enum SocketProtocol {
    TUNNEL = 1,
    TRUSTED = 2,
    STREAM = 3,
}
const SocketProtocolSerdeable = new EnumSerdeable<SocketProtocol>(SocketProtocol);

export class StartServer {
    index = 2;
    descriptor: MessageDescriptor;
    protocol: SocketProtocol;
    port: TunnelPortId;

    constructor(args: { descriptor: MessageDescriptor; protocol: SocketProtocol; port: TunnelPortId }) {
        this.descriptor = args.descriptor;
        this.protocol = args.protocol;
        this.port = args.port;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            descriptor: MessageDescriptor.serdeable,
            protocol: SocketProtocolSerdeable,
            port: TunnelPortId.serdeable,
        }),
        (obj) => new StartServer(obj),
        (obj) => obj,
    );
}

export class CloseServer {
    index = 3;
    descriptor: MessageDescriptor;
    server: ServerDescriptor;

    constructor(args: { descriptor: MessageDescriptor; server: ServerDescriptor }) {
        this.descriptor = args.descriptor;
        this.server = args.server;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ descriptor: MessageDescriptor.serdeable, server: ServerDescriptor.serdeable }),
        (obj) => new CloseServer(obj),
        (obj) => obj,
    );
}

export class ConnectSocket {
    index = 4;
    descriptor: MessageDescriptor;
    protocol: SocketProtocol;
    remote: NodeId;
    port: TunnelPortId;

    constructor(args: { descriptor: MessageDescriptor; protocol: SocketProtocol; remote: NodeId; port: TunnelPortId }) {
        this.descriptor = args.descriptor;
        this.protocol = args.protocol;
        this.remote = args.remote;
        this.port = args.port;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            descriptor: MessageDescriptor.serdeable,
            protocol: SocketProtocolSerdeable,
            remote: NodeId.serdeable,
            port: TunnelPortId.serdeable,
        }),
        (obj) => new ConnectSocket(obj),
        (obj) => obj,
    );
}

export class SendData {
    index = 5;
    descriptor: MessageDescriptor;
    socket: SocketDescriptor;
    payload: Uint8Array;

    constructor(args: { descriptor: MessageDescriptor; socket: SocketDescriptor; payload: Uint8Array }) {
        this.descriptor = args.descriptor;
        this.socket = args.socket;
        this.payload = args.payload;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            descriptor: MessageDescriptor.serdeable,
            socket: SocketDescriptor.serdeable,
            payload: new RemainingBytesSerdeable(),
        }),
        (obj) => new SendData(obj),
        (obj) => obj,
    );
}

export class CloseSocket {
    index = 6;
    descriptor: MessageDescriptor;
    socket: SocketDescriptor;

    constructor(args: { descriptor: MessageDescriptor; socket: SocketDescriptor }) {
        this.descriptor = args.descriptor;
        this.socket = args.socket;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ descriptor: MessageDescriptor.serdeable, socket: SocketDescriptor.serdeable }),
        (obj) => new CloseSocket(obj),
        (obj) => obj,
    );
}

export class Terminate {
    index = 7;
    static readonly serdeable = new TransformSerdeable(
        new EmptySerdeable(),
        () => new Terminate(),
        () => ({}),
    );
}

const requestMessageTypes = [SendHello, StartServer, CloseServer, ConnectSocket, CloseSocket, SendData, Terminate];

export type RequestMessageType = InstanceType<(typeof requestMessageTypes)[number]>;

export class RequestMessage {
    body: RequestMessageType;

    constructor(body: RequestMessageType) {
        this.body = body;
    }

    static readonly serdeable = new TransformSerdeable(
        new VariantSerdeable(
            requestMessageTypes.map((t) => t.serdeable),
            (obj) => obj.index,
        ),
        (obj) => new RequestMessage(obj),
        (obj) => obj.body,
    );
}

export class OperationSuccess {
    index = 1;
    descriptor: MessageDescriptor;

    constructor(descriptor: MessageDescriptor) {
        this.descriptor = descriptor;
    }

    static readonly serdeable = new TransformSerdeable(
        MessageDescriptor.serdeable,
        (descriptor) => new OperationSuccess(descriptor),
        (obj) => obj.descriptor,
    );
}

export class OperationFailure {
    index = 2;
    descriptor: MessageDescriptor;
    trace: string;

    constructor(args: { descriptor: MessageDescriptor; trace: string }) {
        this.descriptor = args.descriptor;
        this.trace = args.trace;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            descriptor: MessageDescriptor.serdeable,
            trace: new Utf8Serdeable(new Uint32Serdeable()),
        }),
        (obj) => new OperationFailure(obj),
        (obj) => obj,
    );
}

export class ServerStarted {
    index = 3;
    descriptor: MessageDescriptor;
    server: ServerDescriptor;

    constructor(args: { descriptor: MessageDescriptor; server: ServerDescriptor }) {
        this.descriptor = args.descriptor;
        this.server = args.server;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ descriptor: MessageDescriptor.serdeable, server: ServerDescriptor.serdeable }),
        (obj) => new ServerStarted(obj),
        (obj) => obj,
    );
}

export class ServerClosed {
    index = 4;
    server: ServerDescriptor;

    constructor(args: { server: ServerDescriptor }) {
        this.server = args.server;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ server: ServerDescriptor.serdeable }),
        (obj) => new ServerClosed(obj),
        (obj) => obj,
    );
}

export class ServerSocketConnected {
    index = 5;
    server: ServerDescriptor;
    socket: SocketDescriptor;
    protocol: SocketProtocol;
    remote: NodeId;
    port: TunnelPortId;

    constructor(args: {
        server: ServerDescriptor;
        socket: SocketDescriptor;
        protocol: SocketProtocol;
        remote: NodeId;
        port: TunnelPortId;
    }) {
        this.server = args.server;
        this.socket = args.socket;
        this.protocol = args.protocol;
        this.remote = args.remote;
        this.port = args.port;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            server: ServerDescriptor.serdeable,
            socket: SocketDescriptor.serdeable,
            protocol: SocketProtocolSerdeable,
            remote: NodeId.serdeable,
            port: TunnelPortId.serdeable,
        }),
        (obj) => new ServerSocketConnected(obj),
        (obj) => obj,
    );
}

export class SocketConnected {
    index = 6;
    descriptor: MessageDescriptor;
    socket: SocketDescriptor;
    protocol: SocketProtocol;
    remote: NodeId;
    port: TunnelPortId;

    constructor(args: {
        descriptor: MessageDescriptor;
        socket: SocketDescriptor;
        protocol: SocketProtocol;
        remote: NodeId;
        port: TunnelPortId;
    }) {
        this.descriptor = args.descriptor;
        this.socket = args.socket;
        this.protocol = args.protocol;
        this.remote = args.remote;
        this.port = args.port;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            descriptor: MessageDescriptor.serdeable,
            socket: SocketDescriptor.serdeable,
            protocol: SocketProtocolSerdeable,
            remote: NodeId.serdeable,
            port: TunnelPortId.serdeable,
        }),
        (obj) => new SocketConnected(obj),
        (obj) => obj,
    );
}

export class SocketReceived {
    index = 7;
    socket: SocketDescriptor;
    payload: Uint8Array;

    constructor(args: { socket: SocketDescriptor; payload: Uint8Array }) {
        this.socket = args.socket;
        this.payload = args.payload;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            socket: SocketDescriptor.serdeable,
            payload: new RemainingBytesSerdeable(),
        }),
        (obj) => new SocketReceived(obj),
        (obj) => obj,
    );
}

export class SocketClosed {
    index = 8;
    socket: SocketDescriptor;

    constructor(args: { socket: SocketDescriptor }) {
        this.socket = args.socket;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ socket: SocketDescriptor.serdeable }),
        (obj) => new SocketClosed(obj),
        (obj) => obj,
    );
}

const responseMessageTypes = [
    OperationSuccess,
    OperationFailure,
    ServerStarted,
    ServerClosed,
    ServerSocketConnected,
    SocketConnected,
    SocketReceived,
    SocketClosed,
];

export type ResponseMessageType = InstanceType<(typeof responseMessageTypes)[number]>;

export class ResponseMessage {
    body: ResponseMessageType;

    constructor(body: ResponseMessageType) {
        this.body = body;
    }

    static readonly serdeable = new TransformSerdeable(
        new VariantSerdeable(
            responseMessageTypes.map((t) => t.serdeable),
            (obj) => obj.index,
        ),
        (body) => new ResponseMessage(body),
        (obj) => obj.body,
    );
}
