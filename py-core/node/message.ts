import { Address, NodeId, TunnelPortId } from "@core/net";
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

export class SendHello {
    index = 1;
    address: Address;

    constructor(args: { address: Address }) {
        this.address = args.address;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ address: Address.serdeable }),
        (obj) => new SendHello(obj),
        (obj) => obj,
    );
}

export class SocketDescriptor {
    descriptor: number;

    constructor(args: { descriptor: number }) {
        this.descriptor = args.descriptor;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ descriptor: new Uint32Serdeable() }),
        (obj) => new SocketDescriptor(obj),
        (obj) => obj,
    );
}

export enum SocketProtocol {
    TUNNEL = 1,
    TRUSTED = 2,
    STREAM = 3,
}
const SocketProtocolSerdeable = new EnumSerdeable<SocketProtocol>(SocketProtocol);

export class OpenSocket {
    index = 2;
    protocol: SocketProtocol;
    port: TunnelPortId;

    constructor(args: { protocol: SocketProtocol; port: TunnelPortId }) {
        this.protocol = args.protocol;
        this.port = args.port;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ protocol: SocketProtocolSerdeable, port: TunnelPortId.serdeable }),
        (obj) => new OpenSocket(obj),
        (obj) => obj,
    );
}

export class ConnectSocket {
    index = 3;
    protocol: SocketProtocol;
    remote: NodeId;
    port: TunnelPortId;

    constructor(args: { protocol: SocketProtocol; remote: NodeId; port: TunnelPortId }) {
        this.protocol = args.protocol;
        this.remote = args.remote;
        this.port = args.port;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            protocol: SocketProtocolSerdeable,
            remote: NodeId.serdeable,
            port: TunnelPortId.serdeable,
        }),
        (obj) => new ConnectSocket(obj),
        (obj) => obj,
    );
}

export class CloseSocket {
    index = 4;
    descriptor: SocketDescriptor;

    constructor(args: { descriptor: SocketDescriptor }) {
        this.descriptor = args.descriptor;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ descriptor: SocketDescriptor.serdeable }),
        (obj) => new CloseSocket(obj),
        (obj) => obj,
    );
}

export class SendData {
    index = 5;
    descriptor: SocketDescriptor;
    payload: Uint8Array;

    constructor(args: { descriptor: SocketDescriptor; payload: Uint8Array }) {
        this.descriptor = args.descriptor;
        this.payload = args.payload;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            descriptor: SocketDescriptor.serdeable,
            payload: new RemainingBytesSerdeable(),
        }),
        (obj) => new SendData(obj),
        (obj) => obj,
    );
}

export class Terminate {
    index = 6;
    static readonly serdeable = new TransformSerdeable(
        new EmptySerdeable(),
        () => new Terminate(),
        () => ({}),
    );
}

export type RequestBodyType = SendHello | OpenSocket | ConnectSocket | CloseSocket | SendData | Terminate;
export class RequestBody {
    index = 1;
    body: RequestBodyType;

    constructor(args: { body: RequestBodyType }) {
        this.body = args.body;
    }

    static readonly serdeable = new VariantSerdeable(
        [
            SendHello.serdeable,
            OpenSocket.serdeable,
            ConnectSocket.serdeable,
            CloseSocket.serdeable,
            SendData.serdeable,
            Terminate.serdeable,
        ],
        (obj) => obj.index,
    );
}

export class OperationSuccess {
    index = 1;

    static readonly serdeable = new TransformSerdeable(
        new EmptySerdeable(),
        () => new OperationSuccess(),
        () => ({}),
    );
}

export class OperationFailure {
    index = 2;
    trace: string;

    constructor(args: { trace: string }) {
        this.trace = args.trace;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ trace: new Utf8Serdeable(new Uint32Serdeable()) }),
        (obj) => new OperationFailure(obj),
        (obj) => obj,
    );
}

export class SocketOpened {
    index = 3;
    descriptor: SocketDescriptor;

    constructor(args: { descriptor: SocketDescriptor }) {
        this.descriptor = args.descriptor;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ descriptor: SocketDescriptor.serdeable }),
        (obj) => new SocketOpened(obj),
        (obj) => obj,
    );
}

export class SocketReceived {
    index = 4;
    descriptor: SocketDescriptor;
    payload: Uint8Array;

    constructor(args: { descriptor: SocketDescriptor; payload: Uint8Array }) {
        this.descriptor = args.descriptor;
        this.payload = args.payload;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            descriptor: SocketDescriptor.serdeable,
            payload: new RemainingBytesSerdeable(),
        }),
        (obj) => new SocketReceived(obj),
        (obj) => obj,
    );
}

export class SocketClosed {
    index = 5;
    descriptor: SocketDescriptor;

    constructor(args: { descriptor: SocketDescriptor }) {
        this.descriptor = args.descriptor;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ descriptor: SocketDescriptor.serdeable }),
        (obj) => new SocketClosed(obj),
        (obj) => obj,
    );
}

export type ResponseBodyType = OperationSuccess | OperationFailure | SocketOpened | SocketReceived | SocketClosed;

export class ResponseBody {
    index = 2;
    body: ResponseBodyType;

    constructor(args: { body: ResponseBodyType }) {
        this.body = args.body;
    }

    static readonly serdeable = new VariantSerdeable(
        [
            OperationSuccess.serdeable,
            OperationFailure.serdeable,
            SocketOpened.serdeable,
            SocketReceived.serdeable,
            SocketClosed.serdeable,
        ],
        (obj) => obj.index,
    );
}

export class MessageDescriptor {
    descriptor: number;

    constructor(args: { descriptor: number }) {
        this.descriptor = args.descriptor;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ descriptor: new Uint32Serdeable() }),
        (obj) => new MessageDescriptor(obj),
        (obj) => obj,
    );
}

export class Message {
    descriptor: MessageDescriptor;
    body: RequestBody | ResponseBody;

    constructor(args: { descriptor: MessageDescriptor; body: RequestBody | ResponseBody }) {
        this.descriptor = args.descriptor;
        this.body = args.body;
    }

    static readonly serdeable = new ObjectSerdeable({
        descriptor: MessageDescriptor.serdeable,
        body: new VariantSerdeable([RequestBody.serdeable, ResponseBody.serdeable], (obj) => obj.index),
    });
}
