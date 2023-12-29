import { Ok } from "oxide.ts";
import {
    DeserializeResult,
    Deserializer,
    MapDeserializer,
    MapSerdeables,
    Reader,
    Serdeable,
    SerializeResult,
    Serializer,
    Writer,
} from "./traits";

type Schema = Record<string, unknown>;

export class ObjectDeserializer<T extends Schema> implements Deserializer<T> {
    #schema: MapDeserializer<T>;

    constructor(schema: MapDeserializer<T>) {
        this.#schema = schema;
    }

    deserialize(reader: Reader): DeserializeResult<T> {
        const obj: Record<string, unknown> = {};
        for (const [key, deserializer] of Object.entries(this.#schema)) {
            const result = deserializer.deserialize(reader);
            if (result.isErr()) {
                return result;
            }
            obj[key] = result.unwrap();
        }
        return Ok(obj as T);
    }
}

export class ObjectSerializer<T extends Schema> implements Serializer {
    #schema: Record<keyof T, Serializer>;

    constructor(schema: Record<keyof T, Serializer>) {
        this.#schema = schema;
    }

    serializedLength(): number {
        return Object.values(this.#schema).reduce((acc, serializer) => acc + serializer.serializedLength(), 0);
    }

    serialize(writer: Writer): SerializeResult {
        for (const serializer of Object.values(this.#schema)) {
            const result = serializer.serialize(writer);
            if (result.isErr()) {
                return result;
            }
        }
        return Ok(undefined);
    }
}

export class ObjectSerdeable<T extends Schema> implements Serdeable<T> {
    #schema: MapSerdeables<T>;

    constructor(schema: MapSerdeables<T>) {
        this.#schema = schema;
    }

    serializer(value: T): Serializer {
        const schema: Record<keyof T, Serializer> = {} as Record<keyof T, Serializer>;
        for (const [k, v] of Object.entries(value)) {
            schema[k as keyof T] = this.#schema[k as keyof T].serializer(v as T[keyof T]);
        }
        return new ObjectSerializer(schema);
    }

    deserializer(): Deserializer<T> {
        const schema: MapDeserializer<T> = {} as MapDeserializer<T>;
        for (const [key, serdeable] of Object.entries(this.#schema)) {
            schema[key as keyof T] = serdeable.deserializer();
        }
        return new ObjectDeserializer(schema);
    }
}
