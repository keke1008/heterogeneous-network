import { Err, Ok } from "oxide.ts";
import { BufferReader, BufferWriter } from "../../buffer";
import { DeserializeResult, InvalidValueError } from "@core/serde";
import { AddressType } from "./type";
import * as z from "zod";

type Octets = readonly [number, number, number, number];
type Port = number;

const portSchema = z.union([z.string().min(1), z.number()]).pipe(z.coerce.number().int().min(0).max(65535));
const stringOctetsSchema = z
    .string()
    .ip({ version: "v4" })
    .transform((v) => v.split(".").map((octet) => parseInt(octet)) as unknown as Octets);
const ipV6LoopbackSchema = z
    .string()
    .regex(/^::1:\d{1,5}$/)
    .transform((v) => v.slice(4))
    .pipe(portSchema)
    .transform((v) => [[127, 0, 0, 1], v] as [Octets, Port]);
const ipV4CompatibleSchema = z
    .string()
    .regex(/^::ffff:(\d{1,3}\.){3}\d{1,3}:\d{1,5}$/)
    .transform((v) => {
        const [, octets, port] = /^::ffff:((?:\d{1,3}\.){3}\d{1,3}):(\d{1,5})$/.exec(v)!;
        return [octets, port];
    })
    .pipe(z.tuple([stringOctetsSchema, portSchema]));
const normalIpVStringSchema = z
    .string()
    .transform((v) => v.split(":"))
    .pipe(z.tuple([stringOctetsSchema, portSchema]));
const numberOctetsSchema = z
    .array(z.number().int().min(0).max(255))
    .length(4)
    .transform((v) => v as unknown as Octets);
const octetsSchema = z.union([stringOctetsSchema, numberOctetsSchema]);
export const ipAddressSchema = z.union([
    ipV6LoopbackSchema,
    ipV4CompatibleSchema,
    normalIpVStringSchema,
    z.tuple([octetsSchema, portSchema]),
]);

export abstract class IpV4Address {
    abstract readonly type: AddressType;
    #octets: Uint8Array;
    #port: number;

    protected constructor(octets: readonly number[] | Uint8Array, port: number) {
        this.#octets = new Uint8Array(octets);
        this.#port = port;
    }

    static deserializeRaw(reader: BufferReader): DeserializeResult<[Octets, number]> {
        const [...octets] = reader.readBytes(4);
        const port = reader.readUint16();
        const result = ipAddressSchema.safeParse([octets, port]);
        return result.success ? Ok(result.data) : Err(new InvalidValueError(result.error.toString()));
    }

    port(): number {
        return this.#port;
    }

    serialize(writer: BufferWriter): void {
        writer.writeBytes(this.#octets);
        writer.writeUint16(this.#port);
    }

    serializedLength(): number {
        return 6;
    }

    equals(other: IpV4Address): boolean {
        return this.#octets.every((octet, index) => octet === other.#octets[index]) && this.#port === other.#port;
    }

    humanReadableString(): string {
        return `${this.#octets.join(".")}:${this.#port}`;
    }

    humanReadableAddress(): string {
        return this.#octets.join(".");
    }

    humanReadablePort(): string {
        return this.#port.toString();
    }

    abstract toString(): string;
}