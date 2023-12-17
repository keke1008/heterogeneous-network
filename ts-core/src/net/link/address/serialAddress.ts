import { BufferReader } from "@core/net/buffer";
import { DeserializeResult } from "@core/serde";
import { SingleByteAddress, schema } from "./singleByte";
import { AddressType } from "./type";

export class SerialAddress extends SingleByteAddress {
    readonly type: AddressType.Serial = AddressType.Serial as const;

    static deserialize(reader: BufferReader): DeserializeResult<SerialAddress> {
        return SingleByteAddress.deserializeRaw(reader).map((address) => new SerialAddress(address));
    }

    static schema = schema.transform((v) => new SerialAddress(v));

    toString(): string {
        return `${this.type}(${this.address()})`;
    }

    display(): string {
        return `SerialAddress(${this.address()})`;
    }
}
