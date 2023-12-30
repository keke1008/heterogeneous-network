import { TransformSerdeable } from "@core/serde";
import { SingleByteAddress, schema } from "./singleByte";
import { AddressType } from "./type";
import { UniqueKey } from "@core/object";

export class SerialAddress extends SingleByteAddress implements UniqueKey {
    readonly type: AddressType.Serial = AddressType.Serial as const;

    static serdeable = new TransformSerdeable(
        SingleByteAddress.rawSerdeable,
        (address) => new SerialAddress(address.address()),
        (address) => new SingleByteAddress(address.address()),
    );

    static schema = schema.transform((v) => new SerialAddress(v));

    uniqueKey(): string {
        return `${this.type}(${this.address()})`;
    }

    toString(): string {
        return `SerialAddress(${this.address()})`;
    }
}
