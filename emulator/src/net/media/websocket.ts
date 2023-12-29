import * as WebSocketFrame from "@core/media/websocket";
import { Address, Frame, FrameHandler, LinkBroadcastError, LinkSendError, Protocol, WebSocketAddress } from "@core/net";
import { Err, Ok, Result } from "oxide.ts/core";

export type ConnectWebsocketError = "already connected" | "could not connect";

class WebSocketConnection implements WebSocketFrame.Connection {
    #socket: WebSocket;
    remote: WebSocketAddress;

    private constructor(socket: WebSocket, remote: WebSocketAddress) {
        this.#socket = socket;
        this.remote = remote;
    }

    static async connect(remote: WebSocketAddress): Promise<Result<WebSocketConnection, ConnectWebsocketError>> {
        const protocol = window.location.protocol === "https:" ? "wss" : "ws";
        const address = `${protocol}://${remote.humanReadableString()}`;

        try {
            const socket = new WebSocket(address);
            await new Promise((resolve, reject) => {
                socket.addEventListener("open", resolve);
                socket.addEventListener("error", reject);
            });

            return Ok(new WebSocketConnection(socket, remote));
        } catch {
            return Err("could not connect");
        }
    }

    send(buffer: Uint8Array): void {
        this.#socket.send(buffer);
    }

    onReceive(callback: (buffer: Uint8Array) => void): void {
        this.#socket.addEventListener("message", async (event) => {
            if (!(event.data instanceof Blob)) {
                console.warn("WebSocket received non-blob data", event.data);
                return;
            }

            callback(new Uint8Array(await event.data.arrayBuffer()));
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

    sendBroadcast?(protocol: Protocol, payload: Uint8Array): Result<void, LinkBroadcastError> {
        return this.#inner.sendBroadcast(protocol, payload);
    }

    onReceive(callback: (frame: Frame) => void): void {
        this.#inner.onReceive(callback);
    }

    onClose(callback: () => void): void {
        this.#inner.onClose(callback);
    }

    async connect(remote: WebSocketAddress): Promise<Result<void, ConnectWebsocketError>> {
        if (this.#inner.hasConnection(remote)) {
            return Err("already connected");
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
