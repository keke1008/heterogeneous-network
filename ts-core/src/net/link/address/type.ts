import { BufferReader } from "@core/net/buffer";
import { DeserializeResult } from "@core/serde";
import { Ok, Err } from "oxide.ts";

export enum AddressType {
    Loopback = 0x7f,
    Serial = 0x01,
    Uhf = 0x02,
    Udp = 0x03,
    WebSocket = 0x04,
}

export const serializeAddressType = (type: AddressType): number => {
    return type;
};

export const deserializeAddressType = (reader: BufferReader): DeserializeResult<AddressType> => {
    const type = reader.readByte();
    if (type in AddressType) {
        return Ok(type);
    } else {
        return Err(new Error(`Invalid address type: ${type}`));
    }
};
