import { Sender } from "@core/channel";
import { TrustedSocket } from "../trusted";
import { Handle, spawn } from "@core/async";
import { Err, Ok, Result } from "oxide.ts";
import { StreamFrame, StreamHeadFrame } from "./frame";
import { BufferReader } from "../buffer";
import { CancelListening, EventBroker } from "@core/event";
import { Progress } from "./progress";
import { StreamPacket } from "./packet";

class SendProgressStorage {
    #sentByteLength = 0;
    #totalByteLength: number;

    #onProgress = new EventBroker<Progress>();
    #onComplete = new EventBroker<Result<void, "timeout" | "invalid operation">>();
    #onCompletePromise = new Promise<Result<void, "timeout" | "invalid operation">>((resolve) =>
        this.#onComplete.listen(resolve),
    );

    constructor(totalByteLength: number) {
        this.#totalByteLength = totalByteLength;
    }

    onProgress(callback: (progress: Progress) => void): CancelListening {
        return this.#onProgress.listen(callback);
    }

    onComplete(callback: () => void): CancelListening {
        return this.#onComplete.listen(callback);
    }

    wait(): Promise<Result<void, "timeout" | "invalid operation">> {
        return this.#onCompletePromise;
    }

    notifySentByteLength(length: number): void {
        this.#sentByteLength += length;
        const progress: Progress = {
            type: "progress",
            doneByteLength: this.#sentByteLength,
            totalByteLength: this.#totalByteLength,
        };
        this.#onProgress.emit(progress);

        if (progress.doneByteLength === progress.totalByteLength) {
            this.#onComplete.emit(Ok(undefined));
        }
    }

    notifyError(error: "timeout" | "invalid operation"): void {
        this.#onComplete.emit(Err(error));
    }
}

export class SendProgress {
    #storage: SendProgressStorage;

    constructor(storage: SendProgressStorage) {
        this.#storage = storage;
    }

    onProgress(callback: (progress: Progress) => void): CancelListening {
        return this.#storage.onProgress(callback);
    }

    onComplete(callback: () => void): CancelListening {
        return this.#storage.onComplete(callback);
    }

    wait(): Promise<Result<void, "timeout" | "invalid operation">> {
        return this.#storage.wait();
    }
}

interface SendEntry {
    frames: StreamFrame[];
    progress: SendProgressStorage;
}

export class StreamSocket {
    #socket: TrustedSocket;

    #sender: Sender<SendEntry> = new Sender();
    #senderHandle: Handle<void>;

    #receiver = new EventBroker<StreamPacket>();
    #receiverHandle: Handle<void>;

    constructor(socket: TrustedSocket) {
        this.#socket = socket;
        this.#senderHandle = spawn(async (signal) => {
            outer: while (!signal.aborted) {
                const data = await this.#sender
                    .receiver()
                    .next(signal)
                    .catch(() => ({ done: true }) as const);
                if (data.done) {
                    break;
                }

                const entry = data.value;
                for (const frame of entry.frames) {
                    const buffer = StreamFrame.serialize(frame);
                    const result = await this.#socket.send(buffer);
                    if (result.isErr()) {
                        entry.progress.notifyError(result.unwrapErr());
                        break outer;
                    }

                    entry.progress.notifySentByteLength(frame.sendDataLength());
                }
            }
        });

        this.#receiverHandle = spawn(async (signal) => {
            const tx = new Sender<Uint8Array>();
            this.#socket.onReceive((data) => tx.send(data));

            const receive = async (): Promise<Uint8Array | undefined> => {
                const buffer = await tx
                    .receiver()
                    .next(signal)
                    .catch(() => ({ done: true }) as const);
                return buffer.done ? undefined : buffer.value;
            };

            while (!signal.aborted) {
                const headBytes = await receive();
                if (headBytes === undefined) {
                    break;
                }

                const head = BufferReader.deserialize(StreamHeadFrame.serdeable.deserializer(), headBytes);
                if (head.isErr()) {
                    console.warn("failed to deserialize stream head frame", head.unwrapErr());
                    break;
                }

                const totalByteLength = head.unwrap().totalByteLength;
                const tx = new Sender<Result<Uint8Array, "invalid packet">>();
                tx.send(Ok(headBytes));

                const packet = new StreamPacket(totalByteLength, tx.receiver());
                this.#receiver.emit(packet);

                while (!packet.isComplete()) {
                    const body = await receive();
                    if (body === undefined) {
                        tx.send(Err("invalid packet"));
                        break;
                    }

                    tx.send(Ok(body));
                }
            }
        });
    }

    async send(data: Uint8Array): Promise<SendProgress> {
        const mtu = await this.#socket.maxPayloadLength();
        const frames = StreamFrame.fromData(data, mtu);
        const progress = new SendProgressStorage(data.length);

        this.#sender.send({ frames, progress });
        return new SendProgress(progress);
    }

    onReceivePacket(callback: (packet: StreamPacket) => void): CancelListening {
        return this.#receiver.listen(callback);
    }

    onClose(callback: () => void): CancelListening {
        return this.#socket.onClose(callback);
    }

    onOpen(callback: () => void): void {
        this.#socket.onOpen(callback);
    }

    async close(): Promise<Result<void, "invalid operation">> {
        this.#senderHandle.cancel();
        await this.#senderHandle;

        this.#receiverHandle.cancel();
        await this.#receiverHandle;

        return this.#socket.close();
    }

    async [Symbol.asyncDispose](): Promise<void> {
        await this.close();
    }
}
