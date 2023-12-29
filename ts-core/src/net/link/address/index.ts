export { LoopbackAddress } from "./loopback";
export { SerialAddress } from "./serialAddress";
export { AddressType } from "./type";
export { UdpAddress } from "./udpAddress";
export { UhfAddress } from "./uhfAddress";
export { WebSocketAddress } from "./webSocketAddress";

import { ManualVariantSerdeable, TransformSerdeable } from "@core/serde";
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

    static readonly table: Record<AddressType, AddressClassConstructor> = {
        [AddressType.Loopback]: LoopbackAddress,
        [AddressType.Serial]: SerialAddress,
        [AddressType.Uhf]: UhfAddress,
        [AddressType.Udp]: UdpAddress,
        [AddressType.WebSocket]: WebSocketAddress,
    };

    static serdeable = new TransformSerdeable(
        new ManualVariantSerdeable<AddressClass>(
            (address) => address.type,
            (index) => Address.table[index as AddressType].serdeable,
        ),
        (address) => new Address(address),
        (address) => address.address,
    );

    equals(other: Address): boolean {
        return this.address.type === other.address.type && this.address.equals(other.address as never);
    }

    toString(): string {
        return this.address.toString();
    }

    display(): string {
        return `Address(${this.address.display()})`;
    }
}
