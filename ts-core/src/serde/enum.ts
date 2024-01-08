import { Ok } from "oxide.ts";
import { DeserializeResult, Deserializer, Reader, Serdeable, Serializer } from "./traits";
import { Uint8Serdeable } from "./primitives";

type Enum = { [key: number]: string };

export class EnumDeserializer<E> implements Deserializer<E> {
    #enum: Enum;
    #base: Deserializer<number>;

    constructor(enm: Enum, base: Deserializer<number>) {
        this.#enum = enm;
        this.#base = base;
    }

    deserialize(reader: Reader): DeserializeResult<E> {
        return this.#base.deserialize(reader).andThen((value) => {
            return value in this.#enum ? Ok(value as unknown as E) : reader.invalidValueError("enum", value);
        });
    }
}

export class EnumSerdeable<E extends number> implements Serdeable<E> {
    #enum: Enum;
    #base: Serdeable<number>;

    constructor(enm: Enum, base: Serdeable<number> = new Uint8Serdeable()) {
        this.#enum = enm;
        this.#base = base;
    }

    serializer(value: E): Serializer {
        if (!(value in this.#enum)) {
            throw new Error("Invalid value for enum");
        }
        return this.#base.serializer(value);
    }

    deserializer(): Deserializer<E> {
        return new EnumDeserializer<E>(this.#enum, this.#base.deserializer());
    }
}
