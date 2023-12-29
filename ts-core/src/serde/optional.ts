import { Ok } from "oxide.ts";
import { BooleanDeserializer, BooleanSerializer } from "./primitives";
import { DeserializeResult, Deserializer, Reader, Serdeable, Serializer } from "./traits";
import { TupleSerializer } from "./tuple";

export class OptionalDeserializer<T> implements Deserializer<T | undefined> {
    #deserializer: Deserializer<T>;

    constructor(deserializer: Deserializer<T>) {
        this.#deserializer = deserializer;
    }

    deserialize(reader: Reader): DeserializeResult<T | undefined> {
        return new BooleanDeserializer()
            .deserialize(reader)
            .andThen((hasValue) => (hasValue ? this.#deserializer.deserialize(reader) : Ok(undefined)));
    }
}

export class OptionalSerdeable<T> implements Serdeable<T | undefined> {
    #serdeable: Serdeable<T>;

    constructor(serdeable: Serdeable<T>) {
        this.#serdeable = serdeable;
    }

    deserializer(): Deserializer<T | undefined> {
        return new OptionalDeserializer(this.#serdeable.deserializer());
    }

    serializer(value: T | undefined): Serializer {
        return value === undefined
            ? new BooleanSerializer(false)
            : new TupleSerializer([new BooleanSerializer(true), this.#serdeable.serializer(value)]);
    }
}
