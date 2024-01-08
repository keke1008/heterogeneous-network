import { match } from "ts-pattern";
import { Ok } from "oxide.ts";
import {
    DeserializableDeserializer,
    DeserializeResult,
    Deserializer,
    Reader,
    SerializableSerializer,
    SerializeResult,
    Serializer,
    Writer,
    Serdeable,
} from "./traits";

export class BooleanDeserializer extends DeserializableDeserializer<boolean> {
    deserialize(reader: Reader): DeserializeResult<boolean> {
        return reader.readByte().andThen((byte) => {
            return match(byte)
                .with(0x00, () => Ok(false))
                .with(0x01, () => Ok(true))
                .otherwise(() => reader.invalidValueError("boolean", byte));
        });
    }
}

export class BooleanSerializer extends SerializableSerializer<boolean> {
    #value: boolean;

    constructor(value: boolean) {
        super(1);
        this.#value = value;
    }

    serialize(writer: Writer): SerializeResult {
        return writer.writeByte(this.#value ? 0x01 : 0x00);
    }
}

export class BooleanSerdeable implements Serdeable<boolean> {
    deserializer(): Deserializer<boolean> {
        return new BooleanDeserializer();
    }

    serializer(value: boolean): Serializer {
        return new BooleanSerializer(value);
    }
}

export class Uint8Deserializer extends DeserializableDeserializer<number> {
    deserialize(reader: Reader): DeserializeResult<number> {
        return reader.readByte();
    }
}

export class Uint8Serializer extends SerializableSerializer<number> {
    #value: number;

    constructor(value: number) {
        super(1);
        this.#value = value;
    }

    serialize(writer: Writer): SerializeResult {
        return writer.writeByte(this.#value);
    }

    serializedLength(): number {
        return 1;
    }
}

export class Uint8Serdeable implements Serdeable<number> {
    deserializer(): Deserializer<number> {
        return new Uint8Deserializer();
    }

    serializer(value: number): Serializer {
        return new Uint8Serializer(value);
    }
}

export class Uint16Deserializer extends DeserializableDeserializer<number> {
    deserialize(reader: Reader): DeserializeResult<number> {
        return reader.readByte().andThen((low) => {
            return reader.readByte().map((high) => {
                return low | (high << 8);
            });
        });
    }
}

export class Uint16Serializer extends SerializableSerializer<number> {
    #value: number;

    constructor(value: number) {
        super(2);
        this.#value = value;
    }

    serialize(writer: Writer): SerializeResult {
        return writer.writeByte(this.#value & 0xff).andThen(() => {
            return writer.writeByte((this.#value >> 8) & 0xff);
        });
    }
}

export class Uint16Serdeable implements Serdeable<number> {
    deserializer(): Deserializer<number> {
        return new Uint16Deserializer();
    }

    serializer(value: number): Serializer {
        return new Uint16Serializer(value);
    }
}

export class Uint32Deserializer extends DeserializableDeserializer<number> {
    deserialize(reader: Reader): DeserializeResult<number> {
        return reader.readByte().andThen((low) => {
            return reader.readByte().andThen((lowMid) => {
                return reader.readByte().andThen((highMid) => {
                    return reader.readByte().map((high) => {
                        return low | (lowMid << 8) | (highMid << 16) | (high << 24);
                    });
                });
            });
        });
    }
}

export class Uint32Serializer extends SerializableSerializer<number> {
    #value: number;

    constructor(value: number) {
        super(4);
        this.#value = value;
    }

    serialize(writer: Writer): SerializeResult {
        return writer.writeByte(this.#value & 0xff).andThen(() => {
            return writer.writeByte((this.#value >> 8) & 0xff).andThen(() => {
                return writer.writeByte((this.#value >> 16) & 0xff).andThen(() => {
                    return writer.writeByte((this.#value >> 24) & 0xff);
                });
            });
        });
    }
}

export class Uint32Serdeable implements Serdeable<number> {
    deserializer(): Deserializer<number> {
        return new Uint32Deserializer();
    }

    serializer(value: number): Serializer {
        return new Uint32Serializer(value);
    }
}
