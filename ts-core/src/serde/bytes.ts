import { DeserializeResult, Deserializer, Reader, Serdeable, SerializeResult, Serializer, Writer } from "./traits";
import { TupleSerdeable, TupleSerializer } from "./tuple";

export class FixedBytesDeserializer implements Deserializer<Uint8Array> {
    #length?: number;

    constructor(length?: number) {
        this.#length = length;
    }

    withLength(length: number): this {
        this.#length = length;
        return this;
    }

    deserialize(reader: Reader): DeserializeResult<Uint8Array> {
        if (this.#length === undefined) {
            throw new Error("BytesDeserializer must be initialized with length");
        }
        return reader.readBytes(this.#length);
    }
}

export class FixedBytesSerializer implements Serializer {
    #bytes: Uint8Array;

    constructor(bytes: Uint8Array) {
        this.#bytes = bytes;
    }

    serializedLength(): number {
        return this.#bytes.length;
    }

    serialize(writer: Writer): SerializeResult {
        return writer.writeBytes(this.#bytes);
    }
}

export class FixedBytesSerdeable implements Serdeable<Uint8Array> {
    #length?: number;

    constructor(length?: number) {
        this.#length = length;
    }

    deserializer(): Deserializer<Uint8Array> {
        return new FixedBytesDeserializer(this.#length);
    }

    serializer(bytes: Uint8Array): Serializer {
        return new FixedBytesSerializer(bytes);
    }
}

export class RemainingBytesDeserializer implements Deserializer<Uint8Array> {
    deserialize(reader: Reader): DeserializeResult<Uint8Array> {
        return reader.readRemainingBytes();
    }
}

export class RemainingBytesSerdeable implements Serdeable<Uint8Array> {
    deserializer(): Deserializer<Uint8Array> {
        return new RemainingBytesDeserializer();
    }

    serializer(bytes: Uint8Array): Serializer {
        return new FixedBytesSerializer(bytes);
    }
}

export class VariableBytesDeserializer implements Deserializer<Uint8Array> {
    #lengthDeserializer: Deserializer<number>;

    constructor(lengthDeserializer: Deserializer<number>) {
        this.#lengthDeserializer = lengthDeserializer;
    }

    deserialize(reader: Reader): DeserializeResult<Uint8Array> {
        return this.#lengthDeserializer.deserialize(reader).andThen((length) => {
            return reader.readBytes(length);
        });
    }
}

export class VariableBytesSerdeable implements Serdeable<Uint8Array> {
    #lengthSerdeable: Serdeable<number>;

    constructor(lengthSerdeable: Serdeable<number>) {
        this.#lengthSerdeable = lengthSerdeable;
    }

    deserializer(): Deserializer<Uint8Array> {
        return new VariableBytesDeserializer(this.#lengthSerdeable.deserializer());
    }

    serializer(bytes: Uint8Array): Serializer {
        return new TupleSerializer([this.#lengthSerdeable.serializer(bytes.length), new FixedBytesSerializer(bytes)]);
    }
}
