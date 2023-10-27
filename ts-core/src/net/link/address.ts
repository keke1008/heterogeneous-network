import { BufferReader, BufferWriter } from "../buffer";

type RawIPv4Address = readonly [number, number, number, number];

const assertValidIPv4Address: (address: readonly number[]) => asserts address is RawIPv4Address = (address) => {
    if (address.length !== 4) {
        throw new Error(`Invalid IPv4 address: ${address}`);
    }
    if (address.find((octet) => octet < 0 || octet > 255)) {
        throw new Error(`Invalid IPv4 address: ${address}`);
    }
};

const assertValidPort = (port: number): void => {
    if (port < 0 || port > 65535) {
        throw new Error(`Invalid port: ${port}`);
    }
};

const parseIPv4Address = (address: string): readonly [number, number, number, number] => {
    if (address === "::1") {
        // ローカル環境だとなぜかIPv6ループバックになっているので、IPv4ループバックに変換する
        return [127, 0, 0, 1];
    }

    const octets = address.split(".").map((octet) => parseInt(octet));
    assertValidIPv4Address(octets);
    return octets as [number, number, number, number];
};

const serializePort = (port: number): [number, number] => {
    assertValidPort(port);
    return [port & 0xff, (port >> 8) & 0xff];
};

const deserializePort = (port: [number, number]): number => {
    return port[0] | (port[1] << 8);
};

export enum AddressType {
    Broadcast = 0xff,
    Serial = 0x01,
    Uhf = 0x02,
    Sinet = 0x03,
    WebSocket = 0x04,
}

export class BroadcastAddress {
    readonly type: AddressType.Broadcast = AddressType.Broadcast as const;

    static deserialize(): BroadcastAddress {
        return new BroadcastAddress();
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

    static deserialize(reader: BufferReader): SerialAddress {
        return new SerialAddress(reader.readByte());
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

    static deserialize(reader: BufferReader): UhfAddress {
        return new UhfAddress(reader.readByte());
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

class IPv4Address {
    readonly address: Uint8Array;
    readonly port: number;

    constructor(address: readonly number[] | Uint8Array, port: number) {
        assertValidIPv4Address([...address]);
        assertValidPort(port);
        this.address = new Uint8Array(address);
        this.port = port;
    }

    static fromHumanReadableString(address: string, port: number): IPv4Address {
        return new IPv4Address(parseIPv4Address(address), port);
    }

    static deserialize(reader: BufferReader): IPv4Address {
        const octets = [...reader.readBytes(6)] as [number, number, number, number, number, number];
        const port = deserializePort([octets[4], octets[5]]);
        return new IPv4Address(octets.slice(0, 4), port);
    }

    serialize(writer: BufferWriter): void {
        writer.writeBytes(this.address);
        const [low, high] = serializePort(this.port);
        writer.writeByte(low);
        writer.writeByte(high);
    }

    serializedLength(): number {
        return 6;
    }

    equals(other: IPv4Address): boolean {
        return this.address.every((octet, index) => octet === other.address[index]) && this.port === other.port;
    }

    humanReadableAddress(): string {
        return `${this.address.join(".")}:${this.port}`;
    }
}

export class SinetAddress extends IPv4Address {
    readonly type: AddressType.Sinet = AddressType.Sinet as const;

    static fromHumanReadableString(address: string, port: number): SinetAddress {
        return new SinetAddress(parseIPv4Address(address), port);
    }

    static deserialize(reader: BufferReader): SinetAddress {
        const addr = IPv4Address.deserialize(reader);
        return new SinetAddress(addr.address, addr.port);
    }

    toString(): string {
        return `${this.type}(${this.humanReadableAddress()})`;
    }
}

export class WebSocketAddress extends IPv4Address {
    readonly type: AddressType.WebSocket = AddressType.WebSocket as const;

    static fromHumanReadableString(address: string, port: number): WebSocketAddress {
        return new WebSocketAddress(parseIPv4Address(address), port);
    }

    static deserialize(reader: BufferReader): WebSocketAddress {
        const addr = IPv4Address.deserialize(reader);
        return new WebSocketAddress(addr.address, addr.port);
    }

    toString(): string {
        return `${this.type}(${this.humanReadableAddress()})`;
    }
}

const numberToAddressClass = (
    number: number,
):
    | typeof BroadcastAddress
    | typeof SerialAddress
    | typeof UhfAddress
    | typeof SinetAddress
    | typeof WebSocketAddress => {
    switch (number) {
        case 0xff:
            return BroadcastAddress;
        case 0x01:
            return SerialAddress;
        case 0x02:
            return UhfAddress;
        case 0x03:
            return SinetAddress;
        case 0x04:
            return WebSocketAddress;
        default:
            throw new Error(`Invalid address type: ${number}`);
    }
};

const addressTypeToNumber = (type: AddressType): number => {
    switch (type) {
        case AddressType.Broadcast:
            return 0xff;
        case AddressType.Serial:
            return 0x01;
        case AddressType.Uhf:
            return 0x02;
        case AddressType.Sinet:
            return 0x03;
        case AddressType.WebSocket:
            return 0x04;
        default:
            throw new Error(`Invalid address type: ${type}`);
    }
};

export type AddressClass = BroadcastAddress | SerialAddress | UhfAddress | SinetAddress | WebSocketAddress;

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

    static deserialize(reader: BufferReader): Address {
        const type = reader.readByte();
        const addressClass = numberToAddressClass(type);
        return new Address(addressClass.deserialize(reader));
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
