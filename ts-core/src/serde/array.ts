import { Ok } from "oxide.ts";
import {
    Deserializable,
    DeserializeResult,
    Deserializer,
    Reader,
    Serdeable,
    Serializable,
    SerializeResult,
    Serializer,
    Writer,
} from "./traits";

export class ArrayDeserializer<T> implements Deserializer<T[]> {
    #deserializable: Deserializable<T>;
    #length?: number;

    constructor(deserializable: Deserializable<T>) {
        this.#deserializable = deserializable;
    }

    withLength(length: number): this {
        this.#length = length;
        return this;
    }

    deserialize(reader: Reader): DeserializeResult<T[]> {
        if (this.#length === undefined) {
            throw new Error("ArrayDeserializer must be initialized with length");
        }

        const deserialized: T[] = [];
        for (let i = 0; i < this.#length; i++) {
            const result = this.#deserializable.deserializer().deserialize(reader);
            if (result.isErr()) {
                return result;
            }
            deserialized.push(result.unwrap());
        }
        return Ok(deserialized);
    }
}

export class ArraySerializer<T> implements Serializer {
    #serializable: Serializable<T>;
    #values: T[];

    constructor(serializable: Serializable<T>, values: T[]) {
        this.#serializable = serializable;
        this.#values = values;
    }

    serializedLength(): number {
        return this.#values.reduce((acc, value) => acc + this.#serializable.serializer(value).serializedLength(), 0);
    }

    serialize(writer: Writer): SerializeResult {
        for (const value of this.#values) {
            const result = this.#serializable.serializer(value).serialize(writer);
            if (result.isErr()) {
                return result;
            }
        }
        return Ok(undefined);
    }
}

export class ArraySerdeable<T> implements Deserializable<T[]>, Serializable<T[]> {
    #serdeable: Serdeable<T>;

    constructor(serdeable: Serdeable<T>) {
        this.#serdeable = serdeable;
    }

    deserializer(): Deserializer<T[]> {
        return new ArrayDeserializer(this.#serdeable);
    }

    serializer(values: T[]): Serializer {
        return new ArraySerializer(this.#serdeable, values);
    }
}
