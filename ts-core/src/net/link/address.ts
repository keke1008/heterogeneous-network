import { Err, Ok, Result } from "oxide.ts";
import { BufferReader, BufferWriter } from "../buffer";

export enum AddressType {
    Broadcast = 0xff,
    Serial = 0x01,
    Uhf = 0x02,
    Udp = 0x03,
}

export class BroadcastAddress {
    readonly type: AddressType.Broadcast = AddressType.Broadcast as const;

    static deserialize(): Result<BroadcastAddress, never> {
        return Ok(new BroadcastAddress());
    }

    serialize(): void {}

    serializedLength(): number {
        return 0;
    }

    equals(): boolean {
        return true;
    }

    toString(): string {
        return `${this.type}()`;
    }
}

export class SerialAddress {
    readonly type: AddressType.Serial = AddressType.Serial as const;
    readonly #address: number;

    constructor(address: number) {
        this.#address = address;
    }

    address(): number {
        return this.#address;
    }

    static deserialize(reader: BufferReader): Result<SerialAddress, never> {
        return Ok(new SerialAddress(reader.readByte()));
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.#address);
    }

    serializedLength(): number {
        return 1;
    }

    equals(other: SerialAddress): boolean {
        return this.#address === other.#address;
    }

    toString(): string {
        return `${this.type}(${this.#address})`;
    }
}

export class UhfAddress {
    readonly type: AddressType.Uhf = AddressType.Uhf as const;
    readonly address: number;

    constructor(address: number) {
        this.address = address;
    }

    static deserialize(reader: BufferReader): Result<UhfAddress, never> {
        return Ok(new UhfAddress(reader.readByte()));
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.address);
    }

    serializedLength(): number {
        return 1;
    }

    equals(other: UhfAddress): boolean {
        return this.address === other.address;
    }

    toString(): string {
        return `${this.type}(${this.address})`;
    }
}

export type UdpAddressError =
    | "number of octets is not 4"
    | "octet is not a number"
    | "octet is not in range 0-255"
    | "port missing"
    | "port exceeds 65535"
    | "port is not a number";

type RawUdpAddress = readonly [number, number, number, number];

export class UdpAddress {
    readonly type: AddressType.Udp = AddressType.Udp as const;
    #octets: Uint8Array;
    #port: number;

    static #checkIpV4Address(octets: readonly number[]): Result<RawUdpAddress, UdpAddressError> {
        if (octets.length !== 4) {
            return Err("number of octets is not 4");
        }
        if (octets.some((octet) => isNaN(octet))) {
            return Err("octet is not a number");
        }
        if (octets.some((octet) => octet < 0 || octet > 255)) {
            return Err("octet is not in range 0-255");
        }
        return Ok(octets as [number, number, number, number]);
    }

    static #checkPort(port: number): Result<number, UdpAddressError> {
        if (isNaN(port)) {
            return Err("port is not a number");
        }
        if (port < 0 || port > 65535) {
            return Err("port exceeds 65535");
        }
        return Ok(port);
    }

    constructor(octets: readonly number[] | Uint8Array, port: number) {
        UdpAddress.#checkIpV4Address([...octets])
            .andThen(() => UdpAddress.#checkPort(port))
            .unwrap();
        this.#octets = new Uint8Array(octets);
        this.#port = port;
    }

    static #serializeHumanReadableIpAddress(address: string): Result<RawUdpAddress, UdpAddressError> {
        if (address === "::1") {
            // ローカル環境だとなぜかIPv6ループバックになっているので、IPv4ループバックに変換する
            return Ok([127, 0, 0, 1]);
        }
        return UdpAddress.#checkIpV4Address(address.split(".").map((octet) => parseInt(octet)));
    }

    static #serializeHumanReadablePort(port: string): Result<number, UdpAddressError> {
        const portNumber = parseInt(port);
        return UdpAddress.#checkPort(portNumber);
    }

    static #serializeHumanReadableIpAddressAndPort(address: string): Result<[RawUdpAddress, number], UdpAddressError> {
        const [ipAddress, port] = address.split(":");
        if (port === undefined) {
            return Err("port missing");
        }

        return Result.all(
            UdpAddress.#serializeHumanReadableIpAddress(ipAddress),
            UdpAddress.#serializeHumanReadablePort(port),
        );
    }

    static fromHumanReadableString(address: string): Result<UdpAddress, UdpAddressError>;
    static fromHumanReadableString(
        octets: string | number[],
        port: string | number,
    ): Result<UdpAddress, UdpAddressError>;
    static fromHumanReadableString(
        ...args: [string] | [string | number[], string | number]
    ): Result<UdpAddress, UdpAddressError> {
        if (args.length === 1) {
            return UdpAddress.#serializeHumanReadableIpAddressAndPort(args[0]).map(
                ([octets, port]) => new UdpAddress(octets, port),
            );
        }

        const [octets, port] = args;
        const parsedOctets =
            typeof octets === "string"
                ? UdpAddress.#serializeHumanReadableIpAddress(octets)
                : UdpAddress.#checkIpV4Address(octets);
        const parsedPort =
            typeof port === "string" ? UdpAddress.#serializeHumanReadablePort(port) : UdpAddress.#checkPort(port);
        return Result.all(parsedOctets, parsedPort).map(([octets, port]) => new UdpAddress(octets, port));
    }

    static #deserializeAddress(reader: BufferReader): Result<RawUdpAddress, UdpAddressError> {
        const octets = reader.readBytes(4);
        return UdpAddress.#checkIpV4Address([...octets]);
    }

    static #deserializePort(reader: BufferReader): Result<number, UdpAddressError> {
        return UdpAddress.#checkPort(reader.readUint16());
    }

    static deserialize(reader: BufferReader): Result<UdpAddress, UdpAddressError> {
        return UdpAddress.#deserializeAddress(reader).andThen((octets) => {
            return UdpAddress.#deserializePort(reader).map((port) => new UdpAddress(octets, port));
        });
    }

    serialize(writer: BufferWriter): void {
        writer.writeBytes(this.#octets);
        writer.writeUint16(this.#port);
    }

    serializedLength(): number {
        return 6;
    }

    equals(other: UdpAddress): boolean {
        return this.#octets.every((octet, index) => octet === other.#octets[index]) && this.#port === other.#port;
    }

    humanReadableString(): string {
        return `${this.#octets.join(".")}:${this.#port}`;
    }

    humanReadableAddress(): string {
        return this.#octets.join(".");
    }
}

const addressTypeToNumber = (type: AddressType): number => {
    switch (type) {
        case AddressType.Broadcast:
            return 0xff;
        case AddressType.Serial:
            return 0x01;
        case AddressType.Uhf:
            return 0x02;
        case AddressType.Udp:
            return 0x03;
        default:
            throw new Error(`Invalid address type: ${type}`);
    }
};

export type AddressClass = BroadcastAddress | SerialAddress | UhfAddress | UdpAddress;
export type AddressClassConstructor =
    | typeof BroadcastAddress
    | typeof SerialAddress
    | typeof UhfAddress
    | typeof UdpAddress;

export type AddressError = UdpAddressError | "invalid address type";

export class Address {
    address: AddressClass;

    constructor(address: AddressClass) {
        this.address = address;
    }

    static broadcast(): Address {
        return new Address(new BroadcastAddress());
    }

    isBroadcast(): boolean {
        return this.address.type === AddressType.Broadcast;
    }

    type(): AddressType {
        return this.address.type;
    }

    static #numberToAddressClass(number: number): Result<AddressClassConstructor, AddressError> {
        const table: Partial<Record<number, AddressClassConstructor>> = {
            0xff: BroadcastAddress,
            0x01: SerialAddress,
            0x02: UhfAddress,
            0x03: UdpAddress,
        };
        const addressClass = table[number];
        return addressClass === undefined ? Err("invalid address type") : Ok(addressClass);
    }

    static deserialize(reader: BufferReader): Result<Address, AddressError> {
        const type = reader.readByte();
        return Address.#numberToAddressClass(type)
            .andThen<AddressClass>((addressClassConstructor) => addressClassConstructor.deserialize(reader))
            .map((address) => new Address(address));
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(addressTypeToNumber(this.address.type));
        this.address.serialize(writer);
    }

    serializedLength(): number {
        return 1 + this.address.serializedLength();
    }

    equals(other: Address): boolean {
        if (this.address.type === other.address.type) {
            return this.address.equals(other.address as never);
        } else {
            return false;
        }
    }

    toString(): string {
        return this.address.toString();
    }
}
