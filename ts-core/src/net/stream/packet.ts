import { Handle, spawn } from "@core/async";
import { IReceiver } from "@core/channel";
import { CancelListening, EventBroker } from "@core/event";
import { DeserializeError, DeserializeResult, Reader } from "@core/serde";
import { Err, Ok, Result } from "oxide.ts";
import { Progress } from "./progress";

class BytesStorage {
    #raw: Uint8Array[] = [];
    #receivedByteLength = 0;
    #totalByteLenght: number;

    constructor(totalSize: number) {
        this.#totalByteLenght = totalSize;
    }

    push(data: Uint8Array) {
        this.#raw.push(data);
        this.#receivedByteLength += data.length;
    }

    progress(): Progress & { type: "progress" } {
        return {
            type: "progress",
            doneByteLength: this.#receivedByteLength,
            totalByteLength: this.#totalByteLenght,
        };
    }

    get buffers(): Uint8Array[] {
        return this.#raw;
    }

    isComplete(): boolean {
        return this.#receivedByteLength === this.#totalByteLenght;
    }
}

export type StreamPacketError = "invalid packet";
export type StreamPacketResult<T> = Result<T, StreamPacketError>;

export class StreamPacket {
    #storage: BytesStorage;
    #receiveHandle: Handle<void>;
    #onProgress = new EventBroker<Progress>();

    constructor(totalByteLength: number, receiver: IReceiver<StreamPacketResult<Uint8Array>>) {
        this.#storage = new BytesStorage(totalByteLength);
        this.#receiveHandle = spawn(async () => {
            for await (const state of receiver) {
                if (state.isErr()) {
                    this.#onProgress.emit({ type: "error", error: "invalid packet" });
                }

                const data = state.unwrap();
                const remaining = totalByteLength - this.#storage.progress().doneByteLength;
                if (data.length > remaining) {
                    throw new Error("Received more bytes than expected");
                }

                this.#storage.push(data);
                this.#onProgress.emit(this.#storage.progress());
            }
        });
    }

    isComplete(): boolean {
        return this.#storage.isComplete();
    }

    onProgress(callback: (progress: Progress) => void): CancelListening {
        return this.#onProgress.listen(callback);
    }

    reader(): PacketReader {
        return new PacketReader(this.#storage);
    }

    async wait(): Promise<StreamPacketResult<PacketReader>> {
        await this.#receiveHandle;
        return this.#storage.isComplete() ? Ok(this.reader()) : Err("invalid packet");
    }
}

export class PacketReader implements Reader {
    #storage: BytesStorage;
    #bufferIndex = 0;
    #byteOffset = 0;

    #errorBuffer() {
        return this.#storage.buffers[this.#bufferIndex] ?? new Uint8Array(0);
    }

    notEnoughBytesError(expected: number, actual: number): Err<DeserializeError> {
        return Err(new DeserializeError.NotEnoughBytesError(this.#errorBuffer(), expected, actual));
    }

    invalidValueError(name: string, value: unknown): Err<DeserializeError> {
        return Err(new DeserializeError.InvalidValueError(this.#errorBuffer(), name, value));
    }

    constructor(storage: BytesStorage) {
        this.#storage = storage;
    }

    readBytes(length: number): DeserializeResult<Uint8Array> {
        const buffers = this.#storage.buffers;
        const result = new Uint8Array(length);

        let bufferOffset = this.#bufferIndex;
        let byteOffset = this.#byteOffset;
        let remaining = length;

        while (remaining > 0) {
            if (bufferOffset >= buffers.length) {
                return this.notEnoughBytesError(buffers.length, bufferOffset);
            }

            const buffer = buffers[bufferOffset];
            const copyLength = Math.min(remaining, buffer.length - byteOffset);
            result.set(buffer.subarray(byteOffset, byteOffset + copyLength), length - remaining);
            remaining -= copyLength;
            byteOffset += copyLength;

            if (byteOffset >= buffer.length) {
                bufferOffset++;
                byteOffset = 0;
            }
        }

        this.#bufferIndex = bufferOffset;
        this.#byteOffset = byteOffset + remaining;

        return Ok(result);
    }

    readByte(): DeserializeResult<number> {
        return this.readBytes(1).map((bytes) => bytes[0]);
    }

    readRemainingBytes(): DeserializeResult<Uint8Array> {
        const buffers = this.#storage.buffers;
        const currentBuffer = buffers[this.#bufferIndex] ?? new Uint8Array(0);
        const remainingBytes = currentBuffer.subarray(this.#byteOffset);
        const remainingBuffers = buffers.slice(this.#bufferIndex + 1);

        const length = remainingBytes.length + remainingBuffers.reduce((acc, buffer) => acc + buffer.length, 0);
        const result = new Uint8Array(length);
        let offset = 0;

        result.set(remainingBytes, offset);
        offset += remainingBytes.length;

        for (const buffer of remainingBuffers) {
            result.set(buffer, offset);
            offset += buffer.length;
        }

        return Ok(result);
    }

    remainingRawBuffers(): Uint8Array[] {
        const buffers = this.#storage.buffers;
        const currentBuffer = buffers[this.#bufferIndex] ?? new Uint8Array(0);
        const remainingBuffers = buffers.slice(this.#bufferIndex + 1);
        const remainingBytes = currentBuffer.subarray(this.#byteOffset);
        return [remainingBytes, ...remainingBuffers];
    }
}
