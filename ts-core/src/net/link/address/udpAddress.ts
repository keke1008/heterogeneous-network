import { AddressType } from "./type";
import { IpV4Address, ipAddressSchema } from "./ipAddress";
import { TransformSerdeable } from "@core/serde";
import { UniqueKey } from "@core/object";

export class UdpAddress extends IpV4Address implements UniqueKey {
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

    uniqueKey(): string {
        return `${this.type}(${this.toHumanReadableString()})`;
    }

    toString(): string {
        return `UdpAddress(${this.toHumanReadableString()})`;
    }
}
