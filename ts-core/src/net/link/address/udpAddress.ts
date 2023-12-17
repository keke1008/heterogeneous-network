import { AddressType } from "./type";
import { IpV4Address, ipAddressSchema } from "./ipAddress";
import { BufferReader } from "@core/net/buffer";
import { DeserializeResult } from "@core/serde";

export class UdpAddress extends IpV4Address {
    readonly type: AddressType.Udp = AddressType.Udp as const;

    static deserialize(reader: BufferReader): DeserializeResult<UdpAddress> {
        return IpV4Address.deserializeRaw(reader).map(([octets, port]) => new UdpAddress(octets, port));
    }

    static schema = ipAddressSchema.transform(([octets, port]) => new UdpAddress(octets, port));

    equals(other: UdpAddress): boolean {
        return super.equals(other);
    }

    toString(): string {
        return `${this.type}(${this.humanReadableString()})`;
    }

    display(): string {
        return `UdpAddress(${this.humanReadableString()})`;
    }
}
