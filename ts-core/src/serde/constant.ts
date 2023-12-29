import { Ok } from "oxide.ts";
import { DeserializeResult, Deserializer, Serdeable, Serializer } from "./traits";
import { consume } from "@core/types";
import { EmptySerializer } from "./empty";

export class ContantDeserializer<T> implements Deserializer<T> {
    #value: T;

    constructor(value: T) {
        this.#value = value;
    }

    deserialize(): DeserializeResult<T> {
        return Ok(this.#value);
    }
}

export class ConstantSerdeable<T> implements Serdeable<T> {
    #value: T;

    constructor(value: T) {
        this.#value = value;
    }

    deserializer(): Deserializer<T> {
        return new ContantDeserializer(this.#value);
    }

    serializer(value: T): Serializer {
        consume(value);
        return new EmptySerializer();
    }
}
