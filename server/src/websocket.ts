import {
    Frame,
    BufferReader,
    deserializeWebSocketFrame,
    serializeWebSocketFrame,
    FrameHandler,
    WebSocketAddress,
    Address,
} from "@core/net";
import { ObjectMap } from "@core/object";
import { WebSocketServer, WebSocket } from "ws";

export class WebSocketHandler implements FrameHandler {
    #selfAddress: WebSocketAddress;
    #sockets: ObjectMap<WebSocketAddress, WebSocket, string> = new ObjectMap((addr) => addr.toString());
    #onReceive: undefined | ((frame: Frame) => void) = undefined;
    #onClose: undefined | (() => void) = undefined;

    constructor(selfAddress: WebSocketAddress) {
        this.#selfAddress = selfAddress;

        const wss = new WebSocketServer({ port: selfAddress.port });
        wss.on("connection", async (ws, req) => {
            if (req.socket.remoteAddress === undefined || req.socket.remotePort === undefined) {
                return;
            }
            const addr = WebSocketAddress.fromHumanReadableString(req.socket.remoteAddress, req.socket.remotePort);
            if (this.#sockets.has(addr)) {
                console.log("WebSocket already connected");
                return;
            }
            this.#sockets.set(addr, ws);

            ws.on("message", (data) => {
                if (!(data instanceof Buffer)) {
                    throw new Error(`Expected Buffer, got ${data}`);
                }

                const frame = deserializeWebSocketFrame(new BufferReader(data));
                this.#onReceive?.(frame);
            });

            ws.on("close", () => {
                console.log("WebSocket closed");
                this.#sockets.delete(addr);
            });

            ws.on("error", (err) => {
                console.log(err);
                throw err;
            });
        });

        wss.on("listening", () => console.log(`WebSocket listening on port ${selfAddress.port}`));
        wss.on("close", () => this.#onClose?.());
        wss.on("error", (err) => console.log(err));
    }

    address(): Address {
        return new Address(this.#selfAddress);
    }

    send(frame: Frame): void {
        if (!(frame.sender.address instanceof WebSocketAddress)) {
            throw new Error(`Expected websocket peer, got ${frame.sender.type}`);
        }

        const ws = this.#sockets.get(frame.sender.address);
        ws?.send(serializeWebSocketFrame(frame));
    }

    onReceive(callback: (frame: Frame) => void): void {
        this.#onReceive = callback;
    }

    onClose(callback: () => void): void {
        this.#onClose = callback;
    }
}
