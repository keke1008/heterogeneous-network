import {
    Address,
    BufferReader,
    BufferWriter,
    Frame,
    FrameHandler,
    LinkBroadcastError,
    LinkSendError,
    Protocol,
    WebSocketAddress,
} from "@core/net";
import { ObjectMap } from "@core/object";
import { Ok, Result } from "oxide.ts";

const PROTOCOL_DESERIALIZED_LENGTH = 1;

export interface WebSocketFrame {
    protocol: Protocol;
    reader: BufferReader;
}

export const deserializeFrame = (reader: BufferReader): WebSocketFrame => {
    const protocol = reader.readByte();
    return { protocol, reader };
};

export const serializeFrame = (frame: WebSocketFrame): BufferReader => {
    const reader = frame.reader.initialized();
    const length = PROTOCOL_DESERIALIZED_LENGTH + reader.remainingLength();
    const writer = new BufferWriter(new Uint8Array(length));

    writer.writeByte(frame.protocol);
    writer.writeBytes(reader.readRemaining());
    return new BufferReader(writer.unwrapBuffer());
};

export interface Connection {
    remote: WebSocketAddress;
    send(buffer: Uint8Array): void;
    onReceive(callback: (buffer: Uint8Array) => void): void;
    onClose(callback: () => void): void;
}

export class WebSocketHandler implements FrameHandler {
    #localAddress?: WebSocketAddress;
    #connections: ObjectMap<WebSocketAddress, Connection, string> = new ObjectMap((a) => a.toString());
    #onReceive?: (frame: Frame) => void;
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

        const reader = serializeFrame({ protocol: frame.protocol, reader: frame.reader });
        const connection = this.#connections.get(frame.remote.address);
        connection?.send(reader.readRemaining());
        return Ok(undefined);
    }

    sendBroadcast(reader: BufferReader): Result<void, LinkBroadcastError> {
        for (const connection of this.#connections.values()) {
            connection.send(reader.initialized().readRemaining());
        }
        return Ok(undefined);
    }

    onReceive(callback: (frame: Frame) => void): void {
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
        connection.onReceive((reader) => {
            const frame = deserializeFrame(new BufferReader(reader));
            this.#onReceive?.({
                remote: new Address(connection.remote),
                protocol: frame.protocol,
                reader: frame.reader,
            });
        });
        connection.onClose(() => {
            this.#connections.delete(connection.remote);
        });
    }

    hasConnection(remote: WebSocketAddress): boolean {
        return this.#connections.has(remote);
    }
}
