import { BufferReader, BufferWriter } from "@core/net/buffer";
import { DeserializeResult } from "@core/serde";
import { Ok } from "oxide.ts";
import * as z from "zod";

export const schema = z.union([z.string().min(1), z.number()]).pipe(z.coerce.number().int().min(0).max(255));

export abstract class SingleByteAddress {
    #address: number;

    protected constructor(address: number) {
        this.#address = address;
    }

    address(): number {
        return this.#address;
    }

    static deserializeRaw(reader: BufferReader): DeserializeResult<number> {
        return Ok(reader.readByte());
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.#address);
    }

    serializedLength(): number {
        return 1;
    }

    equals(other: SingleByteAddress): boolean {
        return this.#address === other.#address;
    }

    toHumanReadableString(): string {
        return this.#address.toString();
    }

    abstract toString(): string;
}
