import { TransformSerdeable } from "@core/serde";
import { SingleByteAddress, schema } from "./singleByte";
import { AddressType } from "./type";

export class UhfAddress extends SingleByteAddress {
    readonly type: AddressType.Uhf = AddressType.Uhf as const;

    static serdeable = new TransformSerdeable(
        SingleByteAddress.rawSerdeable,
        (address) => new UhfAddress(address.address()),
        (address) => new SingleByteAddress(address.address()),
    );

    static schema = schema.transform((v) => new UhfAddress(v));

    toString(): string {
        return `${this.type}(${this.address()})`;
    }

    display(): string {
        return `UhfAddress(${this.address()})`;
    }
}
