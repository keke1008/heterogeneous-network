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

export class TupleDeserializer<Ts extends readonly unknown[]> implements Deserializer<Ts> {
    #deserializer: MapDeserializer<Ts>;

    constructor(deserializables: MapDeserializer<Ts>) {
        this.#deserializer = deserializables;
    }

    deserialize(reader: Reader): DeserializeResult<Ts> {
        const deserialized: unknown[] = [];
        for (const deserializable of this.#deserializer) {
            const result = deserializable.deserialize(reader);
            if (result.isErr()) {
                return result;
            }
            deserialized.push(result.unwrap());
        }
        return Ok(deserialized as unknown as Ts);
    }
}

export class TupleSerializer implements Serializer {
    #serializers: Serializer[];

    constructor(serializers: Serializer[]) {
        this.#serializers = serializers;
    }

    serializedLength(): number {
        return this.#serializers.reduce((acc, serializer) => acc + serializer.serializedLength(), 0);
    }

    serialize(writer: Writer): SerializeResult {
        for (const serializer of this.#serializers) {
            const result = serializer.serialize(writer);
            if (result.isErr()) {
                return result;
            }
        }
        return Ok(undefined);
    }
}

export class TupleSerdeable<Ts extends readonly unknown[]> implements Serdeable<Ts> {
    #serdeables: MapSerdeables<Ts>;

    constructor(serdeable: MapSerdeables<Ts>) {
        this.#serdeables = serdeable;
    }

    deserializer(): Deserializer<Ts> {
        const deserializers = this.#serdeables.map((serdeable) => serdeable.deserializer());
        return new TupleDeserializer<Ts>(deserializers as MapDeserializer<Ts>);
    }

    serializer(values: Ts): Serializer {
        const serializers = this.#serdeables.map((serdeable, i) => serdeable.serializer(values[i]));
        return new TupleSerializer(serializers);
    }
}
