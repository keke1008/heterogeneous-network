import { Sender } from "@core/channel";
import { TrustedSocket } from "../trusted";
import { Handle, spawn } from "@core/async";
import { Err, Ok, Result } from "oxide.ts";
import { StreamFlags, StreamFrame } from "./frame";
import { BufferReader, BufferWriter } from "../buffer";
import { DeserializeError } from "@core/serde";

class FrameAccumulator {
    #payloads: Uint8Array[] = [];

    add(payload: Uint8Array): void {
        this.#payloads.push(payload);
    }

    take(): Uint8Array {
        const length = this.#payloads.reduce((sum, payload) => sum + payload.length, 0);
        const result = new Uint8Array(length);

        let offset = 0;
        for (const payload of this.#payloads) {
            result.set(payload, offset);
            offset += payload.length;
        }

        this.#payloads = [];
        return result;
    }
}

type SendError = "timeout" | "invalid operation";

interface SendEntry {
    data: Uint8Array;
    onSend: (result: Result<void, SendError>) => void;
}

interface Progress {
    total: number;
    done: number;
}

interface ProgressHandle {
    obProgress(callback: (progress: Progress) => void): void;
}

interface SendHandle extends Promise<Result<void, SendError>>, ProgressHandle {}

class InnerSocket {
    #socket: TrustedSocket;

    constructor(socket: TrustedSocket) {
        this.#socket = socket;
    }

    async maxPayloadLength(): Promise<number> {
        const total = await this.#socket.maxPayloadLength();
        return total - StreamFrame.headerLength();
    }

    send(args: { flags: StreamFlags; data: Uint8Array }): Promise<Result<void, "timeout" | "invalid operation">> {
        const buffer = BufferWriter.serialize(StreamFrame.serdeable.serializer(args)).unwrap();
        return this.#socket.send(buffer);
    }

    onReceive(callback: (frame: Result<StreamFrame, DeserializeError>) => void): void {
        this.#socket.onReceive((data) => {
            const frame = BufferReader.deserialize(StreamFrame.serdeable.deserializer(), data);
            callback(frame);
            if (frame.isErr()) {
                console.warn("failed to deserialize stream frame", frame.unwrapErr());
            }
        });
    }

    onClose(callback: () => void): void {
        this.#socket.onClose(callback);
    }

    onOpen(callback: () => void): void {
        this.#socket.onOpen(callback);
    }

    close(): Promise<Result<void, "invalid operation">> {
        return this.#socket.close();
    }

    async [Symbol.asyncDispose](): Promise<void> {
        await this.close();
    }
}

export class StreamSocket {
    #socket: InnerSocket;
    #accumulator = new FrameAccumulator();
    #sender: Sender<SendEntry> = new Sender();
    #senderHandle: Handle<void>;

    constructor(socket: TrustedSocket) {
        this.#socket = new InnerSocket(socket);
        this.#senderHandle = spawn(async (signal) => {
            outer: while (!signal.aborted) {
                const data = await this.#sender
                    .receiver()
                    .next(signal)
                    .catch(() => ({ done: true }) as const);
                if (data.done) {
                    break;
                }

                const maxPayloadLength = await this.#socket.maxPayloadLength();
                const entry = data.value;
                let buffer = entry.data;
                while (buffer.length > 0) {
                    const payload = buffer.subarray(0, maxPayloadLength);
                    buffer = buffer.subarray(maxPayloadLength);
                    const fin = buffer.length === 0;
                    const flags = fin ? StreamFlags.Fin : StreamFlags.None;

                    const result = await this.#socket.send({ flags, data: payload });
                    if (result.isErr()) {
                        const error = result.unwrapErr();
                        console.warn("failed to send data", error);
                        entry.onSend(Err(error));
                        break outer;
                    }
                }

                entry.onSend(Ok(undefined));
            }
        });
    }

    send(data: Uint8Array): Promise<Result<void, "timeout" | "invalid operation">> {
        return new Promise((resolve) => this.#sender.send({ data, onSend: resolve }));
    }

    onReceive(callback: (data: Uint8Array) => void): void {
        this.#socket.onReceive((frame) => {
            if (frame.isErr()) {
                console.warn("failed to deserialize stream frame", frame.unwrapErr());
                return;
            }

            const payload = frame.unwrap().data;
            this.#accumulator.add(payload);
            if (frame.unwrap().flags & StreamFlags.Fin) {
                callback(this.#accumulator.take());
            }
        });
    }

    onClose(callback: () => void): void {
        this.#socket.onClose(callback);
    }

    onOpen(callback: () => void): void {
        this.#socket.onOpen(callback);
    }

    async close(): Promise<Result<void, "invalid operation">> {
        this.#senderHandle.cancel();
        await this.#senderHandle;
        return this.#socket.close();
    }

    async [Symbol.asyncDispose](): Promise<void> {
        await this.close();
    }
}
