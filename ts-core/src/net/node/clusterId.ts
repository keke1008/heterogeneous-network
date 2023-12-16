import { DeserializeResult, DeserializeU8, SerializeU8 } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";

export class ClusterId {
    #id: number;

    private constructor(id: number) {
        this.#id = id;
    }

    static default(): ClusterId {
        return new ClusterId(0);
    }

    equals(other: ClusterId): boolean {
        return this.#id === other.#id;
    }

    static deserialize(reader: BufferReader): DeserializeResult<ClusterId> {
        return DeserializeU8.deserialize(reader).map((id) => new ClusterId(id));
    }

    serialize(writer: BufferWriter) {
        return new SerializeU8(this.#id).serialize(writer);
    }

    serializedLength(): number {
        return new SerializeU8(this.#id).serializedLength();
    }

    toUniqueString(): string {
        return this.#id.toString();
    }
}
