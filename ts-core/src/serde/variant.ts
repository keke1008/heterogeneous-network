import { Err } from "oxide.ts";
import { Uint8Deserializer, Uint8Serializer } from "./primitives";
import { TupleSerializer } from "./tuple";
import { DeserializeResult, Deserializer, MapSerdeables, Reader, Serdeable, Serializer } from "./traits";

export class VariantDeserializer<Ts> implements Deserializer<Ts> {
    #indexToDeserializer: (index: number) => Deserializer<Ts> | undefined;

    constructor(indexToDeserializer: (index: number) => Deserializer<Ts> | undefined) {
        this.#indexToDeserializer = indexToDeserializer;
    }

    deserialize(reader: Reader): DeserializeResult<Ts> {
        return new Uint8Deserializer().deserialize(reader).andThen((index) => {
            const deserializer = this.#indexToDeserializer(index);
            if (deserializer === undefined) {
                return Err(new Error(`No deserializer for index ${index}`));
            }
            return deserializer.deserialize(reader);
        });
    }
}

export class VariantSerdeable<Ts extends readonly unknown[]> implements Serdeable<Ts[number]> {
    #serdeables: MapSerdeables<Ts>;
    #valueToIndex: (value: Ts[number]) => number;
    #offset = 1 as const;

    constructor(serdeables: MapSerdeables<Ts>, valueToIndex: (value: Ts[number]) => number) {
        this.#serdeables = serdeables;
        this.#valueToIndex = valueToIndex;
    }

    serializer(value: Ts[number]): Serializer {
        const index = this.#valueToIndex(value);
        const serdeable = this.#serdeables.at(index);
        if (serdeable === undefined) {
            throw new Error(`No serdeable for index ${index}`);
        }

        return new TupleSerializer([new Uint8Serializer(index + this.#offset), serdeable.serializer(value)]);
    }

    deserializer(): Deserializer<Ts[number]> {
        const deserializers = this.#serdeables.map((serdeable) => serdeable.deserializer());
        return new VariantDeserializer((index) => deserializers[index - this.#offset]);
    }
}

export class ManualVariantSerdeable<Ts> implements Serdeable<Ts> {
    #valueToIndex: (value: Ts) => number;
    #indexToSerdeable: (index: number) => Serdeable<Ts> | undefined;

    constructor(valueToIndex: (value: Ts) => number, indexToSerdeable: (index: number) => Serdeable<Ts>) {
        this.#valueToIndex = valueToIndex;
        this.#indexToSerdeable = indexToSerdeable;
    }

    serializer(value: Ts): Serializer {
        const index = this.#valueToIndex(value);
        const serdeable = this.#indexToSerdeable(index);
        if (serdeable === undefined) {
            throw new Error(`No serdeable for index ${index}`);
        }
        return new TupleSerializer([new Uint8Serializer(index + 1), serdeable.serializer(value)]);
    }

    deserializer(): Deserializer<Ts> {
        return new VariantDeserializer((index) => this.#indexToSerdeable(index)?.deserializer());
    }
}
