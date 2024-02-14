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

type Size = 8 | 16 | 32 | 64;

const createUintDeserializer = (size: Size) => {
    return class extends DeserializableDeserializer<number> {
        deserialize(reader: Reader): DeserializeResult<number> {
            return reader.readBytes(size / 8).map((bytes) => {
                return bytes.reduce((acc, byte, index) => {
                    return acc | (byte << (index * 8));
                }, 0);
            });
        }
    };
};

const createUintSerializer = (size: Size) => {
    return class extends SerializableSerializer<number> {
        #value: number;

        constructor(value: number) {
            super(size / 8);
            this.#value = value;
        }

        serialize(writer: Writer): SerializeResult {
            const bytes = Uint8Array.from({ length: size / 8 }, (_, index) => {
                return (this.#value >> (index * 8)) & 0xff;
            });
            return writer.writeBytes(bytes);
        }
    };
};

const createUintSerdeable = (size: Size) => {
    return class implements Serdeable<number> {
        deserializer(): Deserializer<number> {
            return new (createUintDeserializer(size))();
        }

        serializer(value: number): Serializer {
            return new (createUintSerializer(size))(value);
        }
    };
};

export const Uint8Deserializer = createUintDeserializer(8);
export const Uint8Serializer = createUintSerializer(8);
export const Uint8Serdeable = createUintSerdeable(8);
export const Uint16Deserializer = createUintDeserializer(16);
export const Uint16Serializer = createUintSerializer(16);
export const Uint16Serdeable = createUintSerdeable(16);
export const Uint32Deserializer = createUintDeserializer(32);
export const Uint32Serializer = createUintSerializer(32);
export const Uint32Serdeable = createUintSerdeable(32);
export const Uint64Deserializer = createUintDeserializer(64);
export const Uint64Serializer = createUintSerializer(64);
export const Uint64Serdeable = createUintSerdeable(64);
