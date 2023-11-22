import { Ok, Result } from "oxide.ts";
import { BufferReader, BufferWriter } from "./net";

export interface Serializable {
    serializedLength(): number;
    serialize(writer: BufferWriter): void;
}

interface UntypedDeserializable {
    deserialize(reader: BufferReader): Result<unknown, unknown>;
}

type DeserializeOk<D extends UntypedDeserializable> = ReturnType<ReturnType<D["deserialize"]>["unwrap"]>;
type DeserializeErr<D extends UntypedDeserializable> = ReturnType<ReturnType<D["deserialize"]>["unwrapErr"]>;

export interface Deserializable<T, E> extends UntypedDeserializable {
    deserialize(reader: BufferReader): Result<T, E>;
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

export class DeserializeOptional<D extends Deserializable<DeserializeOk<D>, DeserializeErr<D>>> {
    #deserializable: D;

    constructor(deserializable: D) {
        this.#deserializable = deserializable;
    }

    deserialize(reader: BufferReader): Result<DeserializeOk<D> | undefined, DeserializeErr<D>> {
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

export class DeserializeVector<D extends Deserializable<DeserializeOk<D>, DeserializeErr<D>>>
    implements Deserializable<DeserializeOk<D>[], DeserializeErr<D>>
{
    #deserializable: D;

    constructor(deserializable: D) {
        this.#deserializable = deserializable;
    }

    deserialize(reader: BufferReader): Result<DeserializeOk<D>[], DeserializeErr<D>> {
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
