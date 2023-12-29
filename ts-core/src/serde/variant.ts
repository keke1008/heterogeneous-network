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
        const serdeable = this.#serdeables[index - this.#offset];
        if (serdeable === undefined) {
            throw new Error(`No serdeable for index ${index} with value ${JSON.stringify(value)}`);
        }

        return new TupleSerializer([new Uint8Serializer(index), serdeable.serializer(value)]);
    }

    deserializer(): Deserializer<Ts[number]> {
        return new VariantDeserializer((index) => {
            const serdeable = this.#serdeables[index - this.#offset];
            return serdeable?.deserializer();
        });
    }
}

export class ManualVariantSerdeable<Ts> implements Serdeable<Ts> {
    #valueToId: (value: Ts) => number;
    #idToSerdeable: (id: number) => Serdeable<Ts> | undefined;

    constructor(valueToId: (value: Ts) => number, idToSerdeable: (id: number) => Serdeable<Ts>) {
        this.#valueToId = valueToId;
        this.#idToSerdeable = idToSerdeable;
    }

    serializer(value: Ts): Serializer {
        const id = this.#valueToId(value);
        const serdeable = this.#idToSerdeable(id);
        if (serdeable === undefined) {
            throw new Error(`No serdeable for id ${id}`);
        }
        return new TupleSerializer([new Uint8Serializer(id), serdeable.serializer(value)]);
    }

    deserializer(): Deserializer<Ts> {
        return new VariantDeserializer((id) => this.#idToSerdeable(id)?.deserializer());
    }
}
