import { BufferReader } from "@core/net/buffer";
import { DeserializeResult } from "@core/serde";
import { SingleByteAddress, schema } from "./singleByte";
import { AddressType } from "./type";

export class UhfAddress extends SingleByteAddress {
    readonly type: AddressType.Uhf = AddressType.Uhf as const;

    static deserialize(reader: BufferReader): DeserializeResult<UhfAddress> {
        return SingleByteAddress.deserializeRaw(reader).map((address) => new UhfAddress(address));
    }

    static schema = schema.transform((v) => new UhfAddress(v));

    toString(): string {
        return `${this.type}(${this.address()})`;
    }

    display(): string {
        return `UhfAddress(${this.address()})`;
    }
}
