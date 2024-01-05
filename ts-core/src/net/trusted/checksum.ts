import { SerializeResult, TransformSerdeable, Uint16Serdeable, Writer } from "@core/serde";
import { Ok } from "oxide.ts";

const sumBits = (a: bigint) => a & 0xffffn;
const overflowCount = (a: bigint) => a >> 16n;

class ChecksumCalculator {
    #sum = 0n;

    write(u16: number) {
        if (u16 < 0 || u16 > 0xffff) {
            throw new Error(`Invalid byte: ${u16}`);
        }
        this.#sum += BigInt(u16);
    }

    checksum() {
        return ~(sumBits(this.#sum) + overflowCount(this.#sum)) & 0xffffn;
    }

    verify(): boolean {
        const s = (sumBits(this.#sum) + overflowCount(this.#sum)) & 0xffffn;
        return s === 0xffffn;
    }
}

class WordAccumulator {
    #prevByte: number | undefined;

    write(byte: number): number | undefined {
        if (byte < 0 || byte > 0xff) {
            throw new Error(`Invalid byte: ${byte}`);
        }

        if (this.#prevByte === undefined) {
            this.#prevByte = byte;
            return undefined;
        } else {
            const word = (this.#prevByte << 8) | byte;
            this.#prevByte = undefined;
            return word;
        }
    }

    padding(): number | undefined {
        if (this.#prevByte !== undefined) {
            const word = this.#prevByte << 8;
            this.#prevByte = undefined;
            return word;
        }
    }
}

export class ChecksumWriter implements Writer {
    #wordAccumulator = new WordAccumulator();
    #calculator = new ChecksumCalculator();

    writeByte(byte: number): SerializeResult {
        const word = this.#wordAccumulator.write(byte);
        word && this.#calculator.write(word);
        return Ok(undefined);
    }

    writeBytes(bytes: Uint8Array): SerializeResult {
        bytes.forEach((byte) => this.writeByte(byte));
        return Ok(undefined);
    }

    #writePadding() {
        const word = this.#wordAccumulator.padding();
        word && this.#calculator.write(word);
    }

    checksum(): number {
        this.#writePadding();
        return Number(this.#calculator.checksum());
    }

    verify(): boolean {
        this.#writePadding();
        return this.#calculator.verify();
    }
}

export class Checksum {
    #value: number;

    private constructor(value: number) {
        this.#value = value;
    }

    static zero(): Checksum {
        return new Checksum(0);
    }

    static fromWriter(writer: ChecksumWriter): Checksum {
        return new Checksum(writer.checksum());
    }

    static readonly serdeable = new TransformSerdeable(
        new Uint16Serdeable(),
        (value) => new Checksum(value),
        (checksum) => checksum.#value,
    );
}
