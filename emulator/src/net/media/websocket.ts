import * as WebSocketFrame from "@core/media/websocket";
import {
    Address,
    BufferReader,
    Frame,
    FrameHandler,
    LinkBroadcastError,
    LinkSendError,
    Protocol,
    WebSocketAddress,
} from "@core/net";
import { Err, Ok, Result } from "oxide.ts/core";

export class WebSocketAlreadyConnectedError extends Error {
    constructor() {
        super("WebSocket already connected");
    }
}

class WebSocketConnection implements WebSocketFrame.Connection {
    #socket: WebSocket;
    remote: WebSocketAddress;

    private constructor(socket: WebSocket, remote: WebSocketAddress) {
        this.#socket = socket;
        this.remote = remote;
    }

    static async connect(remote: WebSocketAddress): Promise<Result<WebSocketConnection, Event>> {
        const protocol = window.location.protocol === "https:" ? "wss" : "ws";
        const address = `${protocol}://${remote.humanReadableString()}`;
        const socket = new WebSocket(address);

        try {
            await new Promise((resolve, reject) => {
                socket.addEventListener("open", resolve);
                socket.addEventListener("error", reject);
            });
        } catch (error) {
            return Err(error as Event);
        }

        return Ok(new WebSocketConnection(socket, remote));
    }

    send(buffer: Uint8Array): void {
        this.#socket.send(buffer);
    }

    onReceive(callback: (buffer: Uint8Array) => void): void {
        this.#socket.addEventListener("message", (event) => {
            callback(new Uint8Array(event.data));
        });
    }

    onClose(callback: () => void): void {
        this.#socket.addEventListener("close", callback);
    }
}

export class WebSocketHandler implements FrameHandler {
    #inner: WebSocketFrame.WebSocketHandler;

    constructor(localAddress?: WebSocketAddress) {
        this.#inner = new WebSocketFrame.WebSocketHandler(localAddress);
    }

    address(): Address | undefined {
        return this.#inner.address();
    }

    send(frame: Frame): Result<void, LinkSendError> {
        return this.#inner.send(frame);
    }

    sendBroadcast?(protocol: Protocol, reader: BufferReader): Result<void, LinkBroadcastError> {
        return this.#inner.sendBroadcast(protocol, reader);
    }

    onReceive(callback: (frame: Frame) => void): void {
        this.#inner.onReceive(callback);
    }

    onClose(callback: () => void): void {
        this.#inner.onClose(callback);
    }

    async connect(remote: WebSocketAddress): Promise<Result<void, Event | WebSocketAlreadyConnectedError>> {
        if (this.#inner.hasConnection(remote)) {
            return Err(new WebSocketAlreadyConnectedError());
        }

        const result = await WebSocketConnection.connect(remote);
        if (result.isErr()) {
            return result;
        }

        const connection = result.unwrap();
        this.#inner.addConnection(connection);
        return Ok(undefined);
    }
}
