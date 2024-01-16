import { BufferWriter } from "@core/net/buffer";
import { FixedBytesSerdeable, TransformSerdeable, TupleSerdeable, Uint16Serdeable } from "@core/serde";
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

export class IpV4Address {
    #octets: Uint8Array;
    #port: number;

    protected constructor(octets: readonly number[] | Uint8Array, port: number) {
        this.#octets = new Uint8Array(octets);
        this.#port = port;
    }

    static readonly rawSserdeable = new TransformSerdeable(
        new TupleSerdeable([new FixedBytesSerdeable(4), new Uint16Serdeable()] as const),
        ([octets, port]) => new IpV4Address(octets, port),
        (address) => [address.#octets, address.#port] as const,
    );

    static bodyBytesSize(): number {
        return 6;
    }

    body(): Uint8Array {
        const port = BufferWriter.serialize(new Uint16Serdeable().serializer(this.#port));
        return new Uint8Array([...this.#octets, ...port.unwrap()]);
    }

    port(): number {
        return this.#port;
    }

    octets(): Uint8Array {
        return this.#octets;
    }

    equals(other: IpV4Address): boolean {
        return this.#octets.every((octet, index) => octet === other.#octets[index]) && this.#port === other.#port;
    }

    toHumanReadableString(): string {
        return `${this.#octets.join(".")}:${this.#port}`;
    }

    humanReadableAddress(): string {
        return this.#octets.join(".");
    }

    humanReadablePort(): string {
        return this.#port.toString();
    }
}
