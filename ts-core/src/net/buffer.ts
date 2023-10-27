export class BufferOutOfBoundsException extends Error {
    constructor(buffer: Uint8Array, requested: number) {
        super(`Buffer out of bounds: ${buffer.length} < ${requested}`);
    }
}

export class BufferReader {
    #buffer: Uint8Array;
    #offset = 0;

    constructor(buffer: Uint8Array) {
        this.#buffer = buffer;
    }

    #assertBounds(requested: number) {
        if (this.#offset + requested > this.#buffer.length) {
            throw new BufferOutOfBoundsException(this.#buffer, this.#offset + requested);
        }
    }

    readableLength(): number {
        return this.#buffer.length - this.#offset;
    }

    readByte(): number {
        this.#assertBounds(1);
        return this.#buffer[this.#offset++];
    }

    readBytes(length: number): Uint8Array {
        this.#assertBounds(length);
        const bytes = this.#buffer.subarray(this.#offset, this.#offset + length);
        this.#offset += length;
        return bytes;
    }

    readUint16(): number {
        this.#assertBounds(2);
        const low = this.#buffer[this.#offset++];
        const high = this.#buffer[this.#offset++];
        return low | (high << 8);
    }

    readUntil(predicate: (byte: number) => boolean): Uint8Array {
        const end_index = this.#buffer.findIndex((n) => !predicate(n), this.#offset);
        if (end_index === -1) {
            throw new BufferOutOfBoundsException(this.#buffer, this.#buffer.length + 1);
        }
        const bytes = this.#buffer.subarray(this.#offset, end_index);
        this.#offset = end_index;
        return bytes;
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
        const reader = new BufferReader(this.#buffer);
        this.#offset = 0;
        return reader;
    }
}

export class BufferWriter {
    #buffer: Uint8Array;
    #offset = 0;

    constructor(buffer: Uint8Array) {
        this.#buffer = buffer;
    }

    #assertBounds(requested: number) {
        if (this.#offset + requested > this.#buffer.length) {
            throw new BufferOutOfBoundsException(this.#buffer, this.#offset + requested);
        }
    }

    writableLength(): number {
        return this.#buffer.length - this.#offset;
    }

    writeByte(byte: number): void {
        this.#assertBounds(1);
        this.#buffer[this.#offset++] = byte;
    }

    writeUint16(uint16: number): void {
        this.#assertBounds(2);
        this.#buffer[this.#offset++] = uint16 & 0xff;
        this.#buffer[this.#offset++] = (uint16 >> 8) & 0xff;
    }

    writeBytes(bytes: Uint8Array): void {
        this.#assertBounds(bytes.length);
        this.#buffer.set(bytes, this.#offset);
        this.#offset += bytes.length;
    }

    subWriter(length: number): BufferWriter {
        this.#assertBounds(length);
        const writer = new BufferWriter(this.#buffer.subarray(this.#offset, this.#offset + length));
        this.#offset += length;
        return writer;
    }

    shrinked(): Uint8Array {
        return this.#buffer.subarray(0, this.#offset);
    }

    unwrapBuffer(): Uint8Array {
        if (this.#offset !== this.#buffer.length) {
            throw new Error(`Buffer not fully written: ${this.#offset} < ${this.#buffer.length}`);
        }
        return this.#buffer;
    }
}
