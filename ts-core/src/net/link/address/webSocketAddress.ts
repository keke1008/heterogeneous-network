import { AddressType } from "./type";
import { IpV4Address, ipAddressSchema } from "./ipAddress";
import { TransformSerdeable } from "@core/serde";

export class WebSocketAddress extends IpV4Address {
    readonly type: AddressType.WebSocket = AddressType.WebSocket as const;

    static readonly serdeable = new TransformSerdeable(
        IpV4Address.rawSserdeable,
        (address) => new WebSocketAddress(address.octets(), address.port()),
        (address) => new IpV4Address(address.octets(), address.port()),
    );

    static schema = ipAddressSchema.transform(([octets, port]) => new WebSocketAddress(octets, port));

    static bodyBytesSize(): number {
        return 6;
    }

    equals(other: WebSocketAddress): boolean {
        return super.equals(other);
    }

    toString(): string {
        return `${this.type}(${this.humanReadableString()})`;
    }

    display(): string {
        return `WebSocketAddress(${this.humanReadableString()})`;
    }
}
