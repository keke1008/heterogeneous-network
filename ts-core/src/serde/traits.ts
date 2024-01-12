import { Err, Result } from "oxide.ts";

export class NotEnoughBytesError extends Error {
    constructor(buffer: Uint8Array, expected: number, actual: number) {
        super(`Not enough bytes: expected ${expected}, actual ${actual}, buffer: ${buffer}`);
    }
}
export class InvalidValueError extends Error {
    constructor(buffer: Uint8Array, name: string, value: unknown) {
        super(`Invalid value for ${name}: ${value}, buffer: ${buffer}`);
    }
}
export const DeserializeError = { NotEnoughBytesError, InvalidValueError };
export type DeserializeError = InstanceType<(typeof DeserializeError)[keyof typeof DeserializeError]>;
export type DeserializeResult<T> = Result<T, DeserializeError>;

export interface Reader {
    readByte(): DeserializeResult<number>;
    readBytes(length: number): DeserializeResult<Uint8Array>;
    readRemainingBytes(): DeserializeResult<Uint8Array>;
    notEnoughBytesError(expected: number, actual: number): Err<DeserializeError>;
    invalidValueError(name: string, value: unknown): Err<DeserializeError>;
}

export interface Deserializer<T> {
    deserialize(reader: Reader): DeserializeResult<T>;
}

export interface Deserializable<T> {
    deserializer(): Deserializer<T>;
}

export abstract class DeserializableDeserializer<T> implements Deserializable<T>, Deserializer<T> {
    abstract deserialize(reader: Reader): DeserializeResult<T>;

    deserializer(): Deserializer<T> {
        return this;
    }
}

export type MapDeserializer<Ts> = { [K in keyof Ts]: Deserializer<Ts[K]> };
export type DeserializedValue<T> = T extends Deserializer<infer U> ? U : never;

export class NotEnoughSpaceError extends Error {}
export const SerializeError = { NotEnoughSpaceError };
export type SerializeError = InstanceType<(typeof SerializeError)[keyof typeof SerializeError]>;
export type SerializeResult = Result<void, SerializeError>;

export interface Writer {
    writeByte(byte: number): SerializeResult;
    writeBytes(bytes: Uint8Array): SerializeResult;
}

export interface Serializer {
    serializedLength(): number;
    serialize(writer: Writer): SerializeResult;
}

export interface Serializable<T> {
    serializer(value: T): Serializer;
}

export interface SerdeableCapabilites {
    acceptUndefined: boolean;
}
export const SerdeableCapabilites = {
    mergeDefault: (capabilities?: Partial<SerdeableCapabilites>): SerdeableCapabilites => {
        return {
            acceptUndefined: false,
            ...capabilities,
        };
    },
};

export abstract class SerializableSerializer<T> implements Serializable<T>, Serializer {
    #serializedLength: number;

    constructor(serializedLength: number) {
        this.#serializedLength = serializedLength;
    }

    abstract serialize(writer: Writer): SerializeResult;
    serializedLength(): number {
        return this.#serializedLength;
    }

    serializer(): Serializer {
        return this;
    }
}

export interface Serdeable<T> extends Deserializable<T>, Serializable<T> {
    capabilities?(): SerdeableCapabilites;
}
export type MapSerdeables<Ts> = { [K in keyof Ts]: Serdeable<Ts[K]> };
export type SerdeableValue<T> = T extends Serdeable<infer U> ? U : never;
