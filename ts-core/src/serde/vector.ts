import { Uint8Serdeable } from "./primitives";
import {
    DeserializeResult,
    Serializable,
    Deserializer,
    Reader,
    Serializer,
    Serdeable,
    Writer,
    Deserializable,
} from "./traits";
import { ArrayDeserializer, ArraySerializer } from "./array";
import { TupleSerializer } from "./tuple";

export class VectorDeserializer<T> implements Deserializer<T[]> {
    #lengthDeserializable: Deserializable<number>;
    #deserializable: Deserializable<T>;

    constructor(serdeable: Deserializable<T>, lengthDeserializer: Deserializable<number> = new Uint8Serdeable()) {
        this.#deserializable = serdeable;
        this.#lengthDeserializable = lengthDeserializer;
    }

    deserialize(reader: Reader): DeserializeResult<T[]> {
        return this.#lengthDeserializable
            .deserializer()
            .deserialize(reader)
            .andThen((length) => new ArrayDeserializer(this.#deserializable).withLength(length).deserialize(reader));
    }
}

export class VectorSerializer<T> implements Serializer {
    #serializer: TupleSerializer;

    constructor(
        serializable: Serializable<T>,
        values: T[],
        lengthSerializable: Serializable<number> = new Uint8Serdeable(),
    ) {
        this.#serializer = new TupleSerializer([
            lengthSerializable.serializer(values.length),
            new ArraySerializer(serializable, values),
        ]);
    }

    serializedLength(): number {
        return this.#serializer.serializedLength();
    }

    serialize(writer: Writer): DeserializeResult<void> {
        return this.#serializer.serialize(writer);
    }
}

export class VectorSerdeable<T> implements Serdeable<T[]> {
    #serdeable: Serdeable<T>;
    #lengthSerdeable: Serdeable<number>;

    constructor(serdeable: Serdeable<T>, lengthSerdeable: Serdeable<number> = new Uint8Serdeable()) {
        this.#serdeable = serdeable;
        this.#lengthSerdeable = lengthSerdeable;
    }

    deserializer(): Deserializer<T[]> {
        return new VectorDeserializer(this.#serdeable, this.#lengthSerdeable);
    }

    serializer(values: T[]): Serializer {
        return new VectorSerializer(this.#serdeable, values, this.#lengthSerdeable);
    }
}
