import { TransformSerdeable } from "@core/serde";
import { SingleByteAddress, schema } from "./singleByte";
import { AddressType } from "./type";
import { UniqueKey } from "@core/object";

export class UhfAddress extends SingleByteAddress implements UniqueKey {
    readonly type: AddressType.Uhf = AddressType.Uhf as const;

    static serdeable = new TransformSerdeable(
        SingleByteAddress.rawSerdeable,
        (address) => new UhfAddress(address.address()),
        (address) => new SingleByteAddress(address.address()),
    );

    static schema = schema.transform((v) => new UhfAddress(v));

    uniqueKey(): string {
        return `${this.type}(${this.address()})`;
    }

    toString(): string {
        return `UhfAddress(${this.address()})`;
    }
}
