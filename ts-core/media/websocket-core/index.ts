import {
    Address,
    BufferReader,
    Frame,
    FrameHandler,
    LinkBroadcastError,
    LinkSendError,
    Protocol,
    WebSocketAddress,
    ProtocolSerdeable,
    BufferWriter,
    ReceivedFrame,
} from "@core/net";
import { ObjectMap } from "@core/object";
import { ObjectSerdeable, RemainingBytesSerdeable } from "@core/serde";
import { Ok, Result } from "oxide.ts";

export interface WebSocketFrame {
    protocol: Protocol;
    payload: Uint8Array;
}

const WebSocketFrameSerdeable = new ObjectSerdeable({
    protocol: ProtocolSerdeable,
    payload: new RemainingBytesSerdeable(),
});

export interface Connection {
    remote: WebSocketAddress;
    send(buffer: Uint8Array): void;
    onReceive(callback: (buffer: Uint8Array) => void): void;
    onClose(callback: () => void): void;
}

export class WebSocketHandler implements FrameHandler {
    #localAddress?: WebSocketAddress;
    #connections = new ObjectMap<WebSocketAddress, Connection>();
    #onReceive?: (frame: ReceivedFrame) => void;
    #onClose?: () => void;

    constructor(localAddress?: WebSocketAddress) {
        this.#localAddress = localAddress;
    }

    address(): Address | undefined {
        return this.#localAddress && new Address(this.#localAddress);
    }

    send(frame: Frame): Result<void, LinkSendError> {
        if (!(frame.remote.address instanceof WebSocketAddress)) {
            throw new Error(`Expected UdpAddress, got ${frame.remote}`);
        }

        const buffer = BufferWriter.serialize(WebSocketFrameSerdeable.serializer(frame)).expect(
            "Failed to serialize websocket frame",
        );
        const connection = this.#connections.get(frame.remote.address);
        connection?.send(buffer);
        return Ok(undefined);
    }

    sendBroadcast(protocol: Protocol, payload: Uint8Array): Result<void, LinkBroadcastError> {
        const buffer = BufferWriter.serialize(WebSocketFrameSerdeable.serializer({ protocol, payload })).expect(
            "Failed to serialize websocket frame",
        );
        for (const connection of this.#connections.values()) {
            connection.send(buffer);
        }
        return Ok(undefined);
    }

    onReceive(callback: (frame: ReceivedFrame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive already set");
        }
        this.#onReceive = callback;
    }

    onClose(callback: () => void): void {
        if (this.#onClose !== undefined) {
            throw new Error("onClose already set");
        }
        this.#onClose = callback;
    }

    close(): void {
        this.#onClose?.();
    }

    addConnection(connection: Connection): void {
        this.#connections.set(connection.remote, connection);
        const abortController = new AbortController();
        connection.onReceive((reader) => {
            const result = WebSocketFrameSerdeable.deserializer().deserialize(new BufferReader(reader));
            if (result.isErr()) {
                console.warn(`Failed to deserialize frame: ${result.unwrapErr()}`);
                return;
            }

            const frame = result.unwrap();
            this.#onReceive?.({
                remote: new Address(connection.remote),
                protocol: frame.protocol,
                payload: frame.payload,
                mediaPortAbortSignal: abortController.signal,
            });
        });
        connection.onClose(() => {
            this.#connections.delete(connection.remote);
            abortController.abort();
        });
    }

    hasConnection(remote: WebSocketAddress): boolean {
        return this.#connections.has(remote);
    }
}
