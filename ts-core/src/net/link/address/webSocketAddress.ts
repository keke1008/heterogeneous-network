import { AddressType } from "./type";
import { IpV4Address, ipAddressSchema } from "./ipAddress";
import { BufferReader } from "@core/net/buffer";
import { DeserializeResult } from "@core/serde";

export class WebSocketAddress extends IpV4Address {
    readonly type: AddressType.WebSocket = AddressType.WebSocket as const;

    static deserialize(reader: BufferReader): DeserializeResult<WebSocketAddress> {
        return IpV4Address.deserializeRaw(reader).map(([octets, port]) => new WebSocketAddress(octets, port));
    }

    static schema = ipAddressSchema.transform(([octets, port]) => new WebSocketAddress(octets, port));

    equals(other: WebSocketAddress): boolean {
        return super.equals(other);
    }

    toString(): string {
        return `${this.type}(${this.humanReadableString()})`;
    }
}
