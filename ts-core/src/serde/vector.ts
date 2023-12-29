import { Uint8Deserializer, Uint8Serializer } from "./primitives";
import { DeserializeResult, Serializable, Deserializer, Reader, Serializer, Serdeable, Writer } from "./traits";
import { ArrayDeserializer, ArraySerializer } from "./array";
import { TupleSerializer } from "./tuple";

export class VectorDeserializer<T> implements Deserializer<T[]> {
    #length = new Uint8Deserializer();
    #serdeable: Serdeable<T>;

    constructor(serdeable: Serdeable<T>) {
        this.#serdeable = serdeable;
    }

    deserialize(reader: Reader): DeserializeResult<T[]> {
        return this.#length
            .deserialize(reader)
            .andThen((length) => new ArrayDeserializer(this.#serdeable).withLength(length).deserialize(reader));
    }
}

export class VectorSerializer<T> implements Serializer {
    #serializer: TupleSerializer;

    constructor(serializable: Serializable<T>, values: T[]) {
        this.#serializer = new TupleSerializer([
            new Uint8Serializer(values.length),
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

    constructor(serdeable: Serdeable<T>) {
        this.#serdeable = serdeable;
    }

    deserializer(): Deserializer<T[]> {
        return new VectorDeserializer(this.#serdeable);
    }

    serializer(values: T[]): Serializer {
        return new TupleSerializer([new Uint8Serializer(values.length), new ArraySerializer(this.#serdeable, values)]);
    }
}
