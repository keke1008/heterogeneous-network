import { Err, Ok, Result } from "oxide.ts";
import { BufferReader, BufferWriter } from "../buffer";
import { DeserializeResult, InvalidValueError } from "@core/serde";

export enum AddressType {
    Broadcast = 0xff,
    Serial = 0x01,
    Uhf = 0x02,
    Udp = 0x03,
    WebSocket = 0x04,
}

export type AddressClass = BroadcastAddress | SerialAddress | UhfAddress | UdpAddress | WebSocketAddress;
export type AddressClassConstructor =
    | typeof BroadcastAddress
    | typeof SerialAddress
    | typeof UhfAddress
    | typeof UdpAddress
    | typeof WebSocketAddress;

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
        [AddressType.WebSocket]: WebSocketAddress,
    };
    return table[type];
};

export class BroadcastAddress {
    readonly type: AddressType.Broadcast = AddressType.Broadcast as const;

    static deserialize(): DeserializeResult<BroadcastAddress> {
        return Ok(new BroadcastAddress());
    }

    serialize(): void { }

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

type RawIpV4Address = readonly [number, number, number, number];

export abstract class IpV4Address {
    abstract readonly type: AddressType;
    #octets: Uint8Array;
    #port: number;

    static #checkIpV4Address(octets: readonly number[]): DeserializeResult<RawIpV4Address> {
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

    protected constructor(octets: readonly number[] | Uint8Array, port: number) {
        IpV4Address.#checkIpV4Address([...octets])
            .andThen(() => IpV4Address.#checkPort(port))
            .unwrap();
        this.#octets = new Uint8Array(octets);
        this.#port = port;
    }

    static #deserializeHumanReadableIpAddress(address: string): DeserializeResult<RawIpV4Address> {
        if (address === "::1") {
            // ローカル環境だとなぜかIPv6ループバックになっているので、IPv4ループバックに変換する
            return Ok([127, 0, 0, 1]);
        }
        return IpV4Address.#checkIpV4Address(address.split(".").map((octet) => parseInt(octet)));
    }

    static #deserializeHumanReadablePort(port: string): DeserializeResult<number> {
        const portNumber = parseInt(port);
        return IpV4Address.#checkPort(portNumber);
    }

    static #deserializeHumanReadableIpAddressAndPort(address: string): DeserializeResult<[RawIpV4Address, number]> {
        const [ipAddress, port] = address.split(":");
        if (port === undefined) {
            return Err(new InvalidValueError("port missing"));
        }

        return Result.all(
            IpV4Address.#deserializeHumanReadableIpAddress(ipAddress),
            IpV4Address.#deserializeHumanReadablePort(port),
        );
    }

    static deserializeHumanReadableString(address: string): DeserializeResult<[RawIpV4Address, number]>;
    static deserializeHumanReadableString(
        octets: string | number[],
        port: string | number,
    ): DeserializeResult<[RawIpV4Address, number]>;
    static deserializeHumanReadableString(
        ...args: [string] | [string | number[], string | number]
    ): DeserializeResult<[RawIpV4Address, number]> {
        if (args.length === 1) {
            return IpV4Address.#deserializeHumanReadableIpAddressAndPort(args[0]);
        }

        const [octets, port] = args;
        const parsedOctets =
            typeof octets === "string"
                ? IpV4Address.#deserializeHumanReadableIpAddress(octets)
                : IpV4Address.#checkIpV4Address(octets);
        const parsedPort =
            typeof port === "string" ? IpV4Address.#deserializeHumanReadablePort(port) : IpV4Address.#checkPort(port);
        return Result.all(parsedOctets, parsedPort);
    }

    static deserializeRaw(reader: BufferReader): DeserializeResult<[RawIpV4Address, number]> {
        const octets = reader.readBytes(4);
        const port = reader.readUint16();
        return Result.all(IpV4Address.#checkIpV4Address([...octets]), IpV4Address.#checkPort(port));
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
}

export class UdpAddress extends IpV4Address {
    readonly type: AddressType.Udp = AddressType.Udp as const;

    static deserialize(reader: BufferReader): DeserializeResult<UdpAddress> {
        return IpV4Address.deserializeRaw(reader).map(([octets, port]) => new UdpAddress(octets, port));
    }

    static fromHumanReadableString(address: string): DeserializeResult<UdpAddress>;
    static fromHumanReadableString(octets: string | number[], port: string | number): DeserializeResult<UdpAddress>;
    static fromHumanReadableString(
        ...args: [string] | [string | number[], string | number]
    ): DeserializeResult<UdpAddress> {
        const result =
            args.length === 1
                ? IpV4Address.deserializeHumanReadableString(args[0])
                : IpV4Address.deserializeHumanReadableString(...args);
        return result.map(([octets, port]) => new UdpAddress(octets, port));
    }

    equals(other: UdpAddress): boolean {
        return super.equals(other);
    }

    toString(): string {
        return `${this.type}(${this.humanReadableString()})`;
    }
}
export class WebSocketAddress extends IpV4Address {
    readonly type: AddressType.WebSocket = AddressType.WebSocket as const;

    private constructor(octets: readonly number[] | Uint8Array, port: number) {
        super(octets, port);
    }

    static deserialize(reader: BufferReader): DeserializeResult<WebSocketAddress> {
        return IpV4Address.deserializeRaw(reader).map(([octets, port]) => new WebSocketAddress(octets, port));
    }

    static fromHumanReadableString(address: string): DeserializeResult<WebSocketAddress>;
    static fromHumanReadableString(
        octets: string | number[],
        port: string | number,
    ): DeserializeResult<WebSocketAddress>;
    static fromHumanReadableString(
        ...args: [string] | [string | number[], string | number]
    ): DeserializeResult<WebSocketAddress> {
        const result =
            args.length === 1
                ? IpV4Address.deserializeHumanReadableString(args[0])
                : IpV4Address.deserializeHumanReadableString(...args);
        return result.map(([octets, port]) => new WebSocketAddress(octets, port));
    }

    equals(other: WebSocketAddress): boolean {
        return super.equals(other);
    }

    toString(): string {
        return `${this.type}(${this.humanReadableString()})`;
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
