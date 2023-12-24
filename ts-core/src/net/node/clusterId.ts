import { DeserializeResult, DeserializeU8, InvalidValueError, SerializeU8 } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { Err, Ok } from "oxide.ts";
import * as z from "zod";

const NO_CLUSTER_ID: number = 0;

export class NoCluster {
    serialize(writer: BufferWriter): void {
        return new SerializeU8(NO_CLUSTER_ID).serialize(writer);
    }

    serializedLength(): number {
        return new SerializeU8(NO_CLUSTER_ID).serializedLength();
    }

    isNoCluster(): true {
        return true;
    }

    equals(other: ClusterId | NoCluster): boolean {
        return other instanceof NoCluster;
    }

    display(): string {
        return "NoCluster";
    }
}

export class ClusterId {
    #id: number;

    private constructor(id: number) {
        this.#id = id;
    }

    static NO_CLUSTER_ID: number = 0;

    static noCluster(): NoCluster {
        return new NoCluster();
    }

    isNoCluster(): false {
        return false;
    }

    static fromNumber(id: number): ClusterId | NoCluster {
        return id === ClusterId.NO_CLUSTER_ID ? ClusterId.noCluster() : new ClusterId(id);
    }

    static deserialize(reader: BufferReader): DeserializeResult<ClusterId | NoCluster> {
        return DeserializeU8.deserialize(reader).map((id) => ClusterId.fromNumber(id));
    }

    static noClusterExcludedDeserializer = {
        deserialize: (reader: BufferReader) => {
            return DeserializeU8.deserialize(reader).andThen((id) => {
                return id === ClusterId.NO_CLUSTER_ID
                    ? Err(new InvalidValueError("NoCluster is not allowed"))
                    : Ok(new ClusterId(id));
            });
        },
    };

    static schema = z
        .union([z.string().min(1), z.number()])
        .pipe(z.coerce.number().min(0).max(255))
        .transform(ClusterId.fromNumber);
    static noClusterExcludedSchema = z
        .union([z.string().min(1), z.number()])
        .pipe(z.coerce.number().min(1).max(255))
        .transform((v) => new ClusterId(v));

    serialize(writer: BufferWriter): void {
        return new SerializeU8(this.#id).serialize(writer);
    }

    serializedLength(): number {
        return new SerializeU8(this.#id).serializedLength();
    }

    equals(other: ClusterId | NoCluster): boolean {
        return other instanceof ClusterId && this.#id === other.#id;
    }

    toUniqueString(): string {
        return this.#id.toString();
    }

    toHumanReadableString(): string {
        return this.#id.toString();
    }

    display(): string {
        return `ClusterId(${this.toHumanReadableString()})`;
    }
}

export type OptionalClusterId = ClusterId | NoCluster;
