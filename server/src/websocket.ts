import { Frame, FrameHandler, WebSocketAddress, Address, LinkBroadcastError, LinkSendError, Protocol } from "@core/net";
import * as WebSocketFrame from "@core/media/websocket";
import { WebSocketServer, WebSocket } from "ws";
import { Result } from "oxide.ts";

class WebSocketConnection implements WebSocketFrame.Connection {
    #socket: WebSocket;
    #remote: WebSocketAddress;

    constructor(args: { socket: WebSocket; remote: WebSocketAddress }) {
        this.#socket = args.socket;
        this.#remote = args.remote;
    }

    get remote(): WebSocketAddress {
        return this.#remote;
    }

    send(buffer: Uint8Array): void {
        this.#socket.send(buffer);
    }

    onReceive(callback: (buffer: Uint8Array) => void): void {
        this.#socket.on("message", (data) => {
            if (!(data instanceof Buffer)) {
                console.warn("WebSocket received non-buffer data", data);
                return;
            }
            callback(data);
        });
    }

    onClose(callback: () => void): void {
        this.#socket.on("close", callback);
    }
}

export class WebSocketHandler implements FrameHandler {
    #inner: WebSocketFrame.WebSocketHandler;

    constructor(args: { port: number }) {
        this.#inner = new WebSocketFrame.WebSocketHandler();
        const server = new WebSocketServer({ port: args.port });
        server.on("connection", (socket, req) => {
            const { remoteAddress, remotePort } = req.socket;
            if (remoteAddress === undefined || remotePort === undefined) {
                return;
            }
            console.log(`WebSocket connection from ${remoteAddress}:${remotePort}`);
            const remote = WebSocketAddress.schema.parse(`${remoteAddress}:${remotePort}`);
            this.#inner.addConnection(new WebSocketConnection({ socket, remote }));
        });
        server.on("listening", () => {
            console.log(`WebSocket server listening on port ${args.port}`);
        });
    }

    address(): Address | undefined {
        return this.#inner.address();
    }

    send(frame: Frame): Result<void, LinkSendError> {
        return this.#inner.send(frame);
    }

    sendBroadcast?(protocol: Protocol, payload: Uint8Array): Result<void, LinkBroadcastError> {
        return this.#inner.sendBroadcast(protocol, payload);
    }

    onReceive(callback: (frame: Frame) => void): void {
        this.#inner.onReceive(callback);
    }

    onClose(callback: () => void): void {
        this.#inner.onClose(callback);
    }
}
