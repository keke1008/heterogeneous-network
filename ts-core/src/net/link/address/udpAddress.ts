import { AddressType } from "./type";
import { IpV4Address, ipAddressSchema } from "./ipAddress";
import { TransformSerdeable } from "@core/serde";

export class UdpAddress extends IpV4Address {
    readonly type: AddressType.Udp = AddressType.Udp as const;

    static readonly serdeable = new TransformSerdeable(
        IpV4Address.rawSserdeable,
        (address) => new UdpAddress(address.octets(), address.port()),
        (address) => new IpV4Address(address.octets(), address.port()),
    );

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
