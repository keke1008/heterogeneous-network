import { Err, Ok, Result } from "oxide.ts";
import { BufferReader, BufferWriter } from "../buffer";
import { DeserializeResult, InvalidValueError } from "@core/serde";

export enum AddressType {
    Broadcast = 0xff,
    Serial = 0x01,
    Uhf = 0x02,
    Udp = 0x03,
}

export type AddressClass = BroadcastAddress | SerialAddress | UhfAddress | UdpAddress;
export type AddressClassConstructor =
    | typeof BroadcastAddress
    | typeof SerialAddress
    | typeof UhfAddress
    | typeof UdpAddress;

const serializeAddressType = (type: AddressType): number => {
    return type;
};

const deserializeAddressType = (reader: BufferReader): DeserializeResult<AddressType> => {
    const type = reader.readByte();
    if (type in AddressType) {
        return Ok(type);
    } else {
        return Err(new Error(`Invalid address type: ${type}`));
    }
};

const addressTypeToAddressClass = (type: AddressType): AddressClassConstructor => {
    const table: Record<AddressType, AddressClassConstructor> = {
        [AddressType.Broadcast]: BroadcastAddress,
        [AddressType.Serial]: SerialAddress,
        [AddressType.Uhf]: UhfAddress,
        [AddressType.Udp]: UdpAddress,
    };
    return table[type];
};

export class BroadcastAddress {
    readonly type: AddressType.Broadcast = AddressType.Broadcast as const;

    static deserialize(): DeserializeResult<BroadcastAddress> {
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

    private constructor(address: number) {
        this.#address = address;
    }

    address(): number {
        return this.#address;
    }

    static deserialize(reader: BufferReader): DeserializeResult<SerialAddress> {
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

    static fromString(address: string): DeserializeResult<SerialAddress> {
        return Result.nonNull(parseInt(address))
            .mapErr(() => new InvalidValueError(`Invalid serial address: ${address}`))
            .map((address) => new SerialAddress(address));
    }

    static fromNumber(address: number): DeserializeResult<SerialAddress> {
        if (isNaN(address) || address < 0 || address > 255) {
            return Err(new InvalidValueError(`Invalid serial address: ${address}`));
        } else {
            return Ok(new SerialAddress(address));
        }
    }
}

export class UhfAddress {
    readonly type: AddressType.Uhf = AddressType.Uhf as const;
    readonly address: number;

    constructor(address: number) {
        this.address = address;
    }

    static deserialize(reader: BufferReader): DeserializeResult<UhfAddress> {
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

type RawUdpAddress = readonly [number, number, number, number];

export class UdpAddress {
    readonly type: AddressType.Udp = AddressType.Udp as const;
    #octets: Uint8Array;
    #port: number;

    port(): number {
        return this.#port;
    }

    static #checkIpV4Address(octets: readonly number[]): DeserializeResult<RawUdpAddress> {
        if (octets.length !== 4) {
            return Err(new InvalidValueError("number of octets is not 4"));
        }
        if (octets.some((octet) => isNaN(octet))) {
            return Err(new InvalidValueError("octet is not a number"));
        }
        if (octets.some((octet) => octet < 0 || octet > 255)) {
            return Err(new InvalidValueError("octet is not in range 0-255"));
        }
        return Ok(octets as [number, number, number, number]);
    }

    static #checkPort(port: number): DeserializeResult<number> {
        if (isNaN(port)) {
            return Err(new InvalidValueError("port is not a number"));
        }
        if (port < 0 || port > 65535) {
            return Err(new InvalidValueError("port exceeds 65535"));
        }
        return Ok(port);
    }

    private constructor(octets: readonly number[] | Uint8Array, port: number) {
        UdpAddress.#checkIpV4Address([...octets])
            .andThen(() => UdpAddress.#checkPort(port))
            .unwrap();
        this.#octets = new Uint8Array(octets);
        this.#port = port;
    }

    static #serializeHumanReadableIpAddress(address: string): DeserializeResult<RawUdpAddress> {
        if (address === "::1") {
            // ローカル環境だとなぜかIPv6ループバックになっているので、IPv4ループバックに変換する
            return Ok([127, 0, 0, 1]);
        }
        return UdpAddress.#checkIpV4Address(address.split(".").map((octet) => parseInt(octet)));
    }

    static #serializeHumanReadablePort(port: string): DeserializeResult<number> {
        const portNumber = parseInt(port);
        return UdpAddress.#checkPort(portNumber);
    }

    static #serializeHumanReadableIpAddressAndPort(address: string): DeserializeResult<[RawUdpAddress, number]> {
        const [ipAddress, port] = address.split(":");
        if (port === undefined) {
            return Err(new InvalidValueError("port missing"));
        }

        return Result.all(
            UdpAddress.#serializeHumanReadableIpAddress(ipAddress),
            UdpAddress.#serializeHumanReadablePort(port),
        );
    }

    static fromHumanReadableString(address: string): DeserializeResult<UdpAddress>;
    static fromHumanReadableString(octets: string | number[], port: string | number): DeserializeResult<UdpAddress>;
    static fromHumanReadableString(
        ...args: [string] | [string | number[], string | number]
    ): DeserializeResult<UdpAddress> {
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

    static #deserializeAddress(reader: BufferReader): DeserializeResult<RawUdpAddress> {
        const octets = reader.readBytes(4);
        return UdpAddress.#checkIpV4Address([...octets]);
    }

    static #deserializePort(reader: BufferReader): DeserializeResult<number> {
        return UdpAddress.#checkPort(reader.readUint16());
    }

    static deserialize(reader: BufferReader): DeserializeResult<UdpAddress> {
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

    static deserialize(reader: BufferReader): DeserializeResult<Address> {
        return deserializeAddressType(reader)
            .map(addressTypeToAddressClass)
            .andThen<AddressClass>((addressClassConstructor) => addressClassConstructor.deserialize(reader))
            .map((address) => new Address(address));
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(serializeAddressType(this.address.type));
        this.address.serialize(writer);
    }

    serializedLength(): number {
        return 1 + this.address.serializedLength();
    }

    equals(other: Address): boolean {
        return this.address.type === other.address.type && this.address.equals(other.address as never);
    }

    toString(): string {
        return this.address.toString();
    }
}
