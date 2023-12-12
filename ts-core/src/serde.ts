import { Err, Ok, Result } from "oxide.ts";
import { BufferReader, BufferWriter } from "./net";

export interface Serializable {
    serializedLength(): number;
    serialize(writer: BufferWriter): void;
}

export class NotEnoughBytesError extends Error {}
export class InvalidValueError extends Error {}
export class OtherError extends Error {}

export type DeserializeError = NotEnoughBytesError | InvalidValueError | OtherError;
export type DeserializeResult<T> = Result<T, DeserializeError>;

interface UntypedDeserializable {
    deserialize(reader: BufferReader): Result<unknown, DeserializeError>;
}

type DeserializeOk<D extends UntypedDeserializable> = ReturnType<ReturnType<D["deserialize"]>["unwrap"]>;

export interface Deserializable<T> extends UntypedDeserializable {
    deserialize(reader: BufferReader): Result<T, DeserializeError>;
}

export class SerializeBoolean implements Serializable {
    #value: boolean;

    constructor(value: boolean) {
        this.#value = value;
    }

    serializedLength(): number {
        return 1;
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.#value ? 1 : 0);
    }
}

export class DeserializeBoolean {
    static deserialize(reader: BufferReader): Result<boolean, never> {
        return Ok(reader.readByte() !== 0);
    }
}

export class SerializeU8 implements Serializable {
    #value: number;

    constructor(value: number) {
        this.#value = value;
    }

    serializedLength(): number {
        return 1;
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.#value);
    }
}

export class DeserializeU8 {
    static deserialize(reader: BufferReader): Result<number, never> {
        return Ok(reader.readByte());
    }
}

export class SerializeU16 implements Serializable {
    #value: number;

    constructor(value: number) {
        this.#value = value;
    }

    serializedLength(): number {
        return 2;
    }

    serialize(writer: BufferWriter): void {
        writer.writeUint16(this.#value);
    }
}

export class DeserializeU16 {
    static deserialize(reader: BufferReader): Result<number, never> {
        return Ok(reader.readUint16());
    }
}

export class SerializeBytes implements Serializable {
    #value: Uint8Array;

    constructor(value: Uint8Array) {
        this.#value = value;
    }

    serializedLength(): number {
        return this.#value.length;
    }

    serialize(writer: BufferWriter): void {
        new SerializeU8(this.#value.length).serialize(writer);
        writer.writeBytes(this.#value);
    }
}

export class DeserializeBytes {
    static deserialize(reader: BufferReader): Result<Uint8Array, never> {
        const length = DeserializeU8.deserialize(reader).unwrap();
        return Ok(reader.readBytes(length));
    }
}

export class DeserializeEnum<E> implements Deserializable<Exclude<E, string>> {
    #enum: Record<string, E>;
    #deserializer: Deserializable<number>;

    constructor(enumObject: Record<string, E>, deserializer: Deserializable<number>) {
        this.#enum = enumObject;
        this.#deserializer = deserializer;
    }

    deserialize(reader: BufferReader): DeserializeResult<Exclude<E, string>> {
        return this.#deserializer.deserialize(reader).andThen((value) => {
            return value in this.#enum
                ? Ok(this.#enum[value] as Exclude<E, string>)
                : Err(new InvalidValueError("invalid enum value"));
        });
    }
}

export class SerializeEnum implements Serializable {
    #serializer: Serializable;

    constructor(value: number, serializer: { new (value: number): Serializable }) {
        this.#serializer = new serializer(value);
    }

    serializedLength(): number {
        return this.#serializer.serializedLength();
    }

    serialize(writer: BufferWriter): void {
        this.#serializer.serialize(writer);
    }
}

export class SerializeOptional<S extends Serializable> implements Serializable {
    #serializable: S | undefined;

    constructor(serializable: S | undefined) {
        this.#serializable = serializable;
    }

    serializedLength(): number {
        return 1 + (this.#serializable?.serializedLength() ?? 0);
    }

    serialize(writer: BufferWriter): void {
        new SerializeBoolean(this.#serializable !== undefined).serialize(writer);
        if (this.#serializable) {
            this.#serializable.serialize(writer);
        }
    }
}

export class DeserializeOptional<D extends Deserializable<DeserializeOk<D>>> {
    #deserializable: D;

    constructor(deserializable: D) {
        this.#deserializable = deserializable;
    }

    deserialize(reader: BufferReader): DeserializeResult<DeserializeOk<D> | undefined> {
        const hasValue = DeserializeBoolean.deserialize(reader).unwrap();
        if (hasValue) {
            const deserialized = this.#deserializable.deserialize(reader);
            if (deserialized.isErr()) {
                return deserialized;
            }
            return Ok(deserialized.unwrap());
        }
        return Ok(undefined);
    }
}

export class SerializeVector<S extends Serializable> implements Serializable {
    #serializable: S[];

    constructor(serializable: S[]) {
        this.#serializable = serializable;
    }

    serializedLength(): number {
        return 1 + this.#serializable.reduce((acc, s) => acc + s.serializedLength(), 0);
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.#serializable.length);
        for (const s of this.#serializable) {
            s.serialize(writer);
        }
    }
}

export class DeserializeVector<D extends Deserializable<DeserializeOk<D>>>
    implements Deserializable<DeserializeOk<D>[]>
{
    #deserializable: D;

    constructor(deserializable: D) {
        this.#deserializable = deserializable;
    }

    deserialize(reader: BufferReader): DeserializeResult<DeserializeOk<D>[]> {
        const length = DeserializeU8.deserialize(reader).unwrap();

        const result = [];
        for (let i = 0; i < length; i++) {
            const deserialized = this.#deserializable.deserialize(reader);
            if (deserialized.isErr()) {
                return deserialized;
            }
            result.push(deserialized.unwrap());
        }
        return Ok(result);
    }
}

export class SerializeVariant<Ss extends readonly Serializable[]> implements Serializable {
    #index: number;
    #serializable: Ss[number];

    constructor(index: number, serializable: Ss[number]) {
        this.#index = index;
        this.#serializable = serializable;
    }

    serializedLength(): number {
        return 1 + this.#serializable.serializedLength();
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.#index);
        this.#serializable.serialize(writer);
    }
}

export class DeserializeVariant<Ds extends readonly Deserializable<DeserializeOk<Ds[number]>>[]>
    implements Deserializable<DeserializeOk<Ds[number]>>
{
    #deserializables: Ds;

    constructor(...deserializables: Ds) {
        this.#deserializables = deserializables;
    }

    deserialize(reader: BufferReader): DeserializeResult<DeserializeOk<Ds[number]>> {
        const index = DeserializeU8.deserialize(reader);
        if (index.isErr()) {
            return index;
        }

        const deserializable = this.#deserializables[index.unwrap()];
        if (!deserializable) {
            return Err(new InvalidValueError());
        }

        return deserializable.deserialize(reader);
    }
}

export class SerializeEmpty implements Serializable {
    serializedLength(): number {
        return 0;
    }

    serialize(): void {}
}

export class DeserializeEmpty implements Deserializable<undefined> {
    deserialize(): DeserializeResult<undefined> {
        return Ok(undefined);
    }
}
