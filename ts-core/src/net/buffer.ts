import {
    DeserializeError,
    DeserializeResult,
    Deserializer,
    Reader,
    SerializeError,
    SerializeResult,
    Serializer,
    Writer,
} from "@core/serde";
import { Err, Ok, Result } from "oxide.ts";

export class BufferOutOfBoundsException extends Error {
    constructor(buffer: Uint8Array, requested: number) {
        super(`Buffer out of bounds: ${buffer.length} < ${requested}`);
    }
}

export class BufferWriter implements Writer {
    #buffer: Uint8Array;
    #offset = 0;

    constructor(buffer: Uint8Array) {
        this.#buffer = buffer;
    }

    static serialize<S extends Serializer>(serializer: S): Result<Uint8Array, SerializeError> {
        const length = serializer.serializedLength();
        const writer = new BufferWriter(new Uint8Array(length));
        return serializer.serialize(writer).map(() => writer.unwrapBuffer());
    }

    #requireBounds(requested: number): SerializeResult {
        const expected = this.#offset + requested;
        const actual = this.#buffer.length;
        return expected <= actual ? Ok(undefined) : Err(new Error(`Buffer out of bounds: ${expected} < ${actual}`));
    }

    writeByte(byte: number): SerializeResult {
        return this.#requireBounds(1).map(() => {
            this.#buffer[this.#offset++] = byte;
        });
    }

    writeBytes(bytes: Uint8Array): SerializeResult {
        return this.#requireBounds(bytes.length).map(() => {
            this.#buffer.set(bytes, this.#offset);
            this.#offset += bytes.length;
        });
    }

    unwrapBuffer(): Uint8Array {
        if (this.#offset !== this.#buffer.length) {
            throw new Error(`Buffer not fully written: ${this.#offset} < ${this.#buffer.length}`);
        }
        return this.#buffer;
    }
}

export class BufferReader implements Reader {
    #buffer: Uint8Array;
    #offset = 0;

    constructor(buffer: Uint8Array) {
        this.#buffer = buffer;
    }

    static deserialize<T>(deserializer: Deserializer<T>, buffer: Uint8Array): DeserializeResult<T> {
        return deserializer.deserialize(new BufferReader(buffer));
    }

    #requireBounds(requested: number): DeserializeResult<void> {
        const expected = this.#offset + requested;
        const actual = this.#buffer.length;
        return expected <= actual ? Ok(undefined) : this.notEnoughBytesError(expected, actual);
    }

    readByte(): DeserializeResult<number> {
        return this.#requireBounds(1).map(() => this.#buffer[this.#offset++]);
    }

    readBytes(length: number): DeserializeResult<Uint8Array> {
        return this.#requireBounds(length).map(() => {
            const bytes = this.#buffer.subarray(this.#offset, this.#offset + length);
            this.#offset += length;
            return bytes;
        });
    }

    notEnoughBytesError(expected: number, actual: number): Err<DeserializeError> {
        return Err(new DeserializeError.NotEnoughBytesError(this.#buffer, expected, actual));
    }

    invalidValueError(name: string, value: unknown): Err<DeserializeError> {
        return Err(new DeserializeError.InvalidValueError(this.#buffer, name, value));
    }

    readCount(length: number): DeserializeResult<Uint8Array> {
        return this.#requireBounds(length).map(() => {
            const bytes = this.#buffer.subarray(this.#offset, this.#offset + length);
            this.#offset += length;
            return bytes;
        });
    }

    readRemainingBytes(): DeserializeResult<Uint8Array> {
        return Ok(this.readRemaining());
    }

    readBytesMax(maxLength: number): Uint8Array {
        const readable = this.#buffer.length - this.#offset;
        const length = Math.min(maxLength, readable);
        return this.readCount(length).unwrap();
    }

    readRemaining(): Uint8Array {
        const bytes = this.#buffer.subarray(this.#offset);
        this.#offset = this.#buffer.length;
        return bytes;
    }

    remainingLength(): number {
        return this.#buffer.length - this.#offset;
    }

    initialized(): BufferReader {
        return new BufferReader(this.#buffer);
    }

    subReader(): BufferReader {
        return new BufferReader(this.#buffer.subarray(this.#offset));
    }
}
