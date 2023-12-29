import { DeserializeResult, Deserializer, Reader, Serdeable, SerializeResult, Serializer, Writer } from "./traits";

export class BytesDeserializer implements Deserializer<Uint8Array> {
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

export class RemainingBytesDeserializer implements Deserializer<Uint8Array> {
    deserialize(reader: Reader): DeserializeResult<Uint8Array> {
        return reader.readRemainingBytes();
    }
}

export class BytesSerializer implements Serializer {
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

export class BytesSerdeable implements Serdeable<Uint8Array> {
    #length?: number;

    constructor(length?: number) {
        this.#length = length;
    }

    deserializer(): Deserializer<Uint8Array> {
        return new BytesDeserializer(this.#length);
    }

    serializer(bytes: Uint8Array): Serializer {
        return new BytesSerializer(bytes);
    }
}

export class RemainingBytesSerdeable implements Serdeable<Uint8Array> {
    deserializer(): Deserializer<Uint8Array> {
        return new RemainingBytesDeserializer();
    }

    serializer(bytes: Uint8Array): Serializer {
        return new BytesSerializer(bytes);
    }
}
