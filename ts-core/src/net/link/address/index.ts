export { LoopbackAddress } from "./loopback";
export { SerialAddress } from "./serialAddress";
export { AddressType } from "./type";
export { UdpAddress } from "./udpAddress";
export { UhfAddress } from "./uhfAddress";
export { WebSocketAddress } from "./webSocketAddress";

import { BufferReader, BufferWriter } from "@core/net/buffer";
import { DeserializeResult } from "@core/serde";
import { Ok, Err } from "oxide.ts";
import { LoopbackAddress } from "./loopback";
import { SerialAddress } from "./serialAddress";
import { AddressType } from "./type";
import { UdpAddress } from "./udpAddress";
import { UhfAddress } from "./uhfAddress";
import { WebSocketAddress } from "./webSocketAddress";

export type AddressClass = LoopbackAddress | SerialAddress | UhfAddress | UdpAddress | WebSocketAddress;
export type AddressClassConstructor =
    | typeof LoopbackAddress
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
        [AddressType.Loopback]: LoopbackAddress,
        [AddressType.Serial]: SerialAddress,
        [AddressType.Uhf]: UhfAddress,
        [AddressType.Udp]: UdpAddress,
        [AddressType.WebSocket]: WebSocketAddress,
    };
    return table[type];
};

export class Address {
    address: AddressClass;

    constructor(address: AddressClass) {
        this.address = address;
    }

    static loopback(): Address {
        return new Address(new LoopbackAddress());
    }

    isLoopback(): boolean {
        return this.address.type === AddressType.Loopback;
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
