import { Ok } from "oxide.ts";
import { DeserializeResult, Deserializer, Reader, Serdeable, Serializer } from "./traits";

type Enum = { [key: string]: number } | { [key: number]: string };

export class BitflagsDeserailizer<E> implements Deserializer<E> {
    #all: number;
    #base: Deserializer<number>;

    constructor(enm: Enum, base: Deserializer<number>) {
        this.#all = Object.values(enm)
            .filter((v): v is number => typeof v === "number")
            .reduce((a, b) => a | b, 0);
        this.#base = base;
    }

    deserialize(reader: Reader): DeserializeResult<E> {
        return this.#base.deserialize(reader).andThen((value) => {
            const invalid = value & ~this.#all;
            return invalid ? reader.invalidValueError("bitflags", value) : Ok(value as E);
        });
    }
}

export class BitflagsSerdeable<E extends number> implements Serdeable<E> {
    #enm: Enum;
    #base: Serdeable<number>;

    constructor(enm: Enum, base: Serdeable<number>) {
        this.#enm = enm;
        this.#base = base;
    }

    serializer(value: E): Serializer {
        return this.#base.serializer(value);
    }

    deserializer(): Deserializer<E> {
        return new BitflagsDeserailizer(this.#enm, this.#base.deserializer());
    }
}
