import { z } from "zod";
import { BufferReader, BufferWriter } from "../buffer";
import { DeserializeResult, DeserializeU16, SerializeU16 } from "@core/serde";

export class TunnelPortId {
    #portId: number;

    private constructor(portId: number) {
        this.#portId = portId;
    }

    static readonly schema = z.coerce
        .number()
        .min(0)
        .max(0xff)
        .transform((value) => new TunnelPortId(value));

    static deserialize(reader: BufferReader): DeserializeResult<TunnelPortId> {
        return DeserializeU16.deserialize(reader).map((portId) => new TunnelPortId(portId));
    }

    serialize(writer: BufferWriter): void {
        new SerializeU16(this.#portId).serialize(writer);
    }

    serializedLength(): number {
        return new SerializeU16(this.#portId).serializedLength();
    }

    equals(other: TunnelPortId): boolean {
        return this.#portId === other.#portId;
    }

    toUniqueString(): string {
        return `TunnelPortId(${this.#portId})`;
    }
}
