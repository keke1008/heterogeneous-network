import { Err, Ok } from "oxide.ts";
import { DeserializeResult, Deserializer, Reader, Serdeable, Serializer } from "./traits";

export class TransformDeserializer<T, U> implements Deserializer<U> {
    #deserializer: Deserializer<T>;
    #transform: (value: T) => U | undefined;

    constructor(deserializer: Deserializer<T>, transform: (value: T) => U | undefined) {
        this.#deserializer = deserializer;
        this.#transform = transform;
    }

    deserialize(reader: Reader): DeserializeResult<U> {
        return this.#deserializer.deserialize(reader).andThen((value) => {
            return this.#transform(value) === undefined
                ? Err(new Error(`Transform failed for value ${value}`))
                : Ok(this.#transform(value) as U);
        });
    }
}

export class TransformSerdeable<T, U> implements Serdeable<U> {
    #serdeable: Serdeable<T>;
    #transform: (value: T) => U | undefined;
    #transformBack: (value: U) => T;

    constructor(serdeable: Serdeable<T>, transform: (value: T) => U | undefined, transformBack: (value: U) => T) {
        this.#serdeable = serdeable;
        this.#transform = transform;
        this.#transformBack = transformBack;
    }

    serializer(value: U): Serializer {
        return this.#serdeable.serializer(this.#transformBack(value));
    }

    deserializer(): Deserializer<U> {
        return new TransformDeserializer(this.#serdeable.deserializer(), this.#transform);
    }
}
