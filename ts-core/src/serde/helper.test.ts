import { Err, Ok } from "oxide.ts";
import { DeserializeError, DeserializeResult, Reader, SerializeError, SerializeResult, Writer } from "./traits";

export class TestWriter implements Writer {
    bytes: Uint8Array;
    writtenBytes: number = 0;

    constructor(length: number) {
        this.bytes = new Uint8Array(length);
    }

    requireBounds(length: number): SerializeResult {
        const expected = this.writtenBytes + length;
        const actual = this.bytes.length;
        if (expected > actual) {
            return Err(new SerializeError.NotEnoughSpaceError());
        }

        return Ok(undefined);
    }

    writeByte(byte: number): SerializeResult {
        return this.requireBounds(1).andThen(() => {
            this.bytes[this.writtenBytes] = byte;
            this.writtenBytes += 1;
            return Ok(undefined);
        });
    }

    writeBytes(bytes: Uint8Array): SerializeResult {
        return this.requireBounds(bytes.length).andThen(() => {
            this.bytes.set(bytes, this.writtenBytes);
            this.writtenBytes += bytes.length;
            return Ok(undefined);
        });
    }
}

describe("TestWriter", () => {
    it("writes bytes", () => {
        const writer = new TestWriter(2);
        expect(writer.writeByte(0x01).isOk()).toBe(true);
        expect(writer.writeByte(0x02).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([0x01, 0x02]));
    });

    it("fails to write bytes if there is not enough space", () => {
        const writer = new TestWriter(1);
        expect(writer.writeByte(0x01).isOk()).toBe(true);
        expect(writer.writeByte(0x02).isOk()).toBe(false);
    });

    it("writes bytes with writeBytes", () => {
        const writer = new TestWriter(2);
        expect(writer.writeBytes(new Uint8Array([0x01, 0x02])).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([0x01, 0x02]));
    });

    it("fails to write bytes with writeBytes if there is not enough space", () => {
        const writer = new TestWriter(1);
        expect(writer.writeBytes(new Uint8Array([0x01, 0x02])).isOk()).toBe(false);
    });
});

export class TestReader implements Reader {
    bytes: Uint8Array;
    readCount: number = 0;

    constructor(bytes: number[] | Uint8Array) {
        this.bytes = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);
    }

    notEnoughBytesError(expected: number, actual: number): Err<DeserializeError> {
        return Err(new DeserializeError.NotEnoughBytesError(this.bytes, expected, actual));
    }

    invalidValueError(name: string, value: unknown): Err<DeserializeError> {
        return Err(new DeserializeError.InvalidValueError(this.bytes, name, value));
    }

    requireBounds(length: number): DeserializeResult<void> {
        const expected = this.readCount + length;
        const actual = this.bytes.length;
        if (expected > actual) {
            return Err(new DeserializeError.NotEnoughBytesError(this.bytes, expected, actual));
        }

        return Ok(undefined);
    }

    readByte(): DeserializeResult<number> {
        return this.requireBounds(1).andThen(() => {
            const byte = this.bytes[this.readCount];
            this.readCount += 1;
            return Ok(byte);
        });
    }

    readBytes(length: number): DeserializeResult<Uint8Array> {
        return this.requireBounds(length).andThen(() => {
            const bytes = this.bytes.slice(this.readCount, this.readCount + length);
            this.readCount += length;
            return Ok(bytes);
        });
    }

    readRemainingBytes(): DeserializeResult<Uint8Array> {
        return this.readBytes(this.bytes.length - this.readCount);
    }
}

describe("TestReader", () => {
    it("reads bytes", () => {
        const reader = new TestReader(new Uint8Array([0x01, 0x02]));
        expect(reader.readByte().unwrap()).toBe(0x01);
        expect(reader.readByte().unwrap()).toBe(0x02);
    });

    it("fails to read bytes if there is not enough bytes", () => {
        const reader = new TestReader(new Uint8Array([0x01]));
        expect(reader.readByte().isOk()).toBe(true);
        expect(reader.readByte().isOk()).toBe(false);
    });

    it("reads bytes with readBytes", () => {
        const reader = new TestReader(new Uint8Array([0x01, 0x02]));
        expect(reader.readBytes(2).unwrap()).toEqual(new Uint8Array([0x01, 0x02]));
    });

    it("fails to read bytes with readBytes if there is not enough bytes", () => {
        const reader = new TestReader(new Uint8Array([0x01]));
        expect(reader.readBytes(2).isOk()).toBe(false);
    });

    it("reads remaining bytes", () => {
        const reader = new TestReader(new Uint8Array([0x01, 0x02]));
        expect(reader.readRemainingBytes().unwrap()).toEqual(new Uint8Array([0x01, 0x02]));
    });

    it("reads remaining bytes with readBytes", () => {
        const reader = new TestReader(new Uint8Array([0x01, 0x02]));
        expect(reader.readBytes(1).unwrap()).toEqual(new Uint8Array([0x01]));
        expect(reader.readRemainingBytes().unwrap()).toEqual(new Uint8Array([0x02]));
    });
});
