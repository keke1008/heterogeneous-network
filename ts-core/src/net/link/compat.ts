import { DeserializeResult, DeserializeU8, SerializeU8 } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import * as z from "zod";

export class MediaPortNumber {
    #value: number;

    constructor(value: number) {
        this.#value = value;
    }

    static deserialize(reader: BufferReader): DeserializeResult<MediaPortNumber> {
        return DeserializeU8.deserialize(reader).map((value) => new MediaPortNumber(value));
    }

    serialize(writer: BufferWriter) {
        new SerializeU8(this.#value).serialize(writer);
    }

    serializedLength() {
        return new SerializeU8(this.#value).serializedLength();
    }

    static schema = z
        .union([z.number(), z.string().min(1)])
        .pipe(z.coerce.number().min(0).max(255))
        .transform((value) => new MediaPortNumber(value));
}
