import {
    AddressType,
    Destination,
    NetFacade,
    NetFacadeBuilder,
    NodeId,
    StreamSocket,
    TrustedSocket,
    TunnelService,
    TunnelSocket,
    UdpAddress,
} from "@core/net";
import { UdpHandler, getLocalIpV4Addresses } from "@media/udp-node";
import { WebSocketHandler } from "@media/websocket-node";
import {
    CloseServer,
    CloseSocket,
    ConnectSocket,
    OperationFailure,
    OperationSuccess,
    ResponseMessageType,
    SendData,
    SendHello,
    ServerClosed,
    ServerDescriptor,
    ServerStarted,
    ServerSocketConnected,
    SocketClosed,
    SocketConnected,
    SocketDescriptor,
    SocketProtocol,
    SocketReceived,
    StartServer,
} from "./message";
import { Err, Ok, Result } from "oxide.ts";
import { SingleListenerEventBroker } from "@core/event";
import { PostingServer } from "@core/apps/posting";
import { P, match } from "ts-pattern";

const UDP_PORT = 12347;
const WEBSOCKET_PORT = 12348;

type SocketType = TunnelSocket | TrustedSocket | StreamSocket;

class Socket {
    #inner: SocketType;
    #onClose?: () => void;

    constructor(inner: SocketType) {
        this.#inner = inner;
    }

    #handleClose() {
        this.#inner.close();
        this.#onClose?.();
        this.#onClose = undefined;
    }

    onClose(callback: () => void) {
        if (this.#onClose !== undefined) {
            throw new Error("Callback already set");
        }
        this.#onClose = callback;
    }

    send(data: Uint8Array): Promise<Result<void, string>> {
        return match(this.#inner)
            .with(P.instanceOf(TunnelSocket), async (s) => {
                const res = await s.send(data);
                return res.isOk() ? Ok(undefined) : Err(res.unwrapErr()?.type ?? "Unknown error");
            })
            .otherwise(async (s) => s.send(data));
    }

    onReceive(callback: (data: Uint8Array) => void) {
        match(this.#inner)
            .with(P.instanceOf(TunnelSocket), (s) => s.receiver().forEach((frame) => callback(frame.data)))
            .otherwise((s) => s.onReceive(callback));
    }

    close() {
        this.#handleClose();
    }
}

interface Server {
    close(): void;
}

class Descriptor<T> {
    #factory: { new (value: number): T };
    #next = 0;

    constructor(factory: { new (value: number): T }) {
        this.#factory = factory;
    }

    next(): T {
        return new this.#factory(this.#next++);
    }
}

export class NetApps {
    #posting: PostingServer;

    constructor(args: { tunnelServer: TunnelService }) {
        this.#posting = new PostingServer(args);
        this.#posting.onReceive((data) => {
            console.log("PostingServer received: ", data);
        });
    }

    close(): void {
        this.#posting.close();
    }
}

export class NetCore {
    #net: NetFacade;
    #apps: NetApps;

    #sockets: Map<number, Socket> = new Map();
    #socketDiscriptor = new Descriptor<SocketDescriptor>(SocketDescriptor);

    #servers: Map<number, Server> = new Map();
    #serverDescriptor = new Descriptor<ServerDescriptor>(ServerDescriptor);

    #response = new SingleListenerEventBroker<ResponseMessageType>();

    #getService(protocol: SocketProtocol) {
        return match(protocol)
            .with(SocketProtocol.TUNNEL, () => this.#net.tunnel())
            .with(SocketProtocol.TRUSTED, () => this.#net.trusted())
            .with(SocketProtocol.STREAM, () => this.#net.stream())
            .run();
    }

    constructor() {
        this.#net = new NetFacadeBuilder().buildWithDefaults();
        this.#net.addHandler(AddressType.Udp, new UdpHandler(UDP_PORT));
        this.#net.addHandler(AddressType.WebSocket, new WebSocketHandler({ port: WEBSOCKET_PORT }));

        const ipAddr = getLocalIpV4Addresses()[0];
        const localAddress = UdpAddress.schema.parse([ipAddr, UDP_PORT]);
        this.#net.localNode().tryInitialize(NodeId.fromAddress(localAddress));

        this.#apps = new NetApps({ tunnelServer: this.#net.tunnel() });
    }

    listenResponse(callback: (mes: ResponseMessageType) => void) {
        this.#response.listen(callback);
    }

    async sendHello(mes: SendHello): Promise<OperationSuccess | OperationFailure> {
        const res = await this.#net.neighbor().sendHello(mes.address, mes.cost);
        return res.isOk()
            ? new OperationSuccess(mes.descriptor)
            : new OperationFailure({ descriptor: mes.descriptor, trace: res.unwrapErr().type });
    }

    async startServer(mes: StartServer): Promise<ServerStarted | OperationFailure> {
        const server_descriptor = this.#serverDescriptor.next();
        const close = this.#getService(mes.protocol).listen(mes.port, (socket) => {
            const socket_descriptor = this.#socketDiscriptor.next();
            const s = new Socket(socket);
            this.#sockets.set(socket_descriptor.value, s);

            s.onReceive((data) => {
                this.#response.emit(new SocketReceived({ socket: socket_descriptor, payload: data }));
            });

            s.onClose(() => {
                this.#sockets.delete(socket_descriptor.value);
                this.#response.emit(new SocketClosed({ socket: socket_descriptor }));
            });

            this.#response.emit(
                new ServerSocketConnected({
                    server: server_descriptor,
                    socket: socket_descriptor,
                    protocol: mes.protocol,
                    remote: socket.destination.nodeId,
                    port: socket.destinationPortId,
                }),
            );
        });

        if (close.isErr()) {
            return new OperationFailure({
                descriptor: mes.descriptor,
                trace: close.unwrapErr(),
            });
        }

        this.#servers.set(server_descriptor.value, { close: close.unwrap() });
        return new ServerStarted({
            descriptor: mes.descriptor,
            server: server_descriptor,
        });
    }

    async closeServer(mes: CloseServer): Promise<OperationSuccess | OperationFailure> {
        const server = this.#servers.get(mes.server.descriptor);
        if (server === undefined) {
            return new OperationFailure({
                descriptor: mes.descriptor,
                trace: "Server not found",
            });
        }

        server.close();
        this.#servers.delete(mes.server.descriptor);
        this.#response.emit(new ServerClosed({ server: mes.server }));
        return new OperationSuccess(mes.descriptor);
    }

    async connectSocket(mes: ConnectSocket): Promise<SocketConnected | OperationFailure> {
        const socket: Result<SocketType, string> = await this.#getService(mes.protocol).connect({
            destination: Destination.fromNodeId(mes.remote),
            destinationPortId: mes.port,
        });

        if (socket.isErr()) {
            return new OperationFailure({
                descriptor: mes.descriptor,
                trace: socket.unwrapErr(),
            });
        }

        const s = new Socket(socket.unwrap());
        const socket_descriptor = this.#socketDiscriptor.next();
        this.#sockets.set(socket_descriptor.value, s);
        s.onReceive((data) => {
            this.#response.emit(new SocketReceived({ socket: socket_descriptor, payload: data }));
        });

        s.onClose(() => {
            this.#sockets.delete(socket_descriptor.value);
            this.#response.emit(new SocketClosed({ socket: socket_descriptor }));
        });

        return new SocketConnected({
            descriptor: mes.descriptor,
            socket: socket_descriptor,
            protocol: mes.protocol,
            remote: mes.remote,
            port: mes.port,
        });
    }

    async closeSocket(mes: CloseSocket): Promise<OperationSuccess | OperationFailure> {
        const socket = this.#sockets.get(mes.socket.descriptor);
        if (socket === undefined) {
            return new OperationFailure({
                descriptor: mes.descriptor,
                trace: "Socket not found",
            });
        }

        socket.close();
        this.#sockets.delete(mes.socket.descriptor);
        return new OperationSuccess(mes.descriptor);
    }

    async sendData(mes: SendData): Promise<OperationSuccess | OperationFailure> {
        const socket = this.#sockets.get(mes.socket.descriptor);
        if (socket === undefined) {
            return new OperationFailure({
                descriptor: mes.descriptor,
                trace: "Socket not found",
            });
        }

        const res = await socket.send(mes.payload);
        return res.isOk()
            ? new OperationSuccess(mes.descriptor)
            : new OperationFailure({ descriptor: mes.descriptor, trace: res.unwrapErr() });
    }

    terminate(): void {
        this.#apps.close();
        this.#net.dispose();
    }
}
