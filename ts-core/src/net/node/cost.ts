import { BufferReader, BufferWriter } from "../buffer";
import { Err, Ok, Result } from "oxide.ts";
import { DeserializeResult } from "@core/serde";

export class Cost {
    #cost: number;

    constructor(cost: number) {
        if (cost < 0 || cost > 0xffff) {
            throw new Error(`Cost must be between 0 and 65535, but was ${cost}`);
        }
        this.#cost = Math.floor(cost);
    }

    static fromNumber(cost: number): Result<Cost, void> {
        if (cost < 0 || cost > 0xffff) {
            return Err(undefined);
        }
        return Ok(new Cost(cost));
    }

    get(): number {
        return this.#cost;
    }

    equals(other: Cost): boolean {
        return this.#cost === other.#cost;
    }

    add(other: Cost): Cost {
        return new Cost(this.#cost + other.#cost);
    }

    lessThan(other: Cost): boolean {
        return this.#cost < other.#cost;
    }

    static deserialize(reader: BufferReader): DeserializeResult<Cost> {
        return Ok(new Cost(reader.readUint16()));
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.#cost & 0xff);
        writer.writeByte(this.#cost & 0xff00);
    }

    serializedLength(): number {
        return 2;
    }
}
