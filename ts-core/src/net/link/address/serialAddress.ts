import { TransformSerdeable } from "@core/serde";
import { SingleByteAddress, schema } from "./singleByte";
import { AddressType } from "./type";

export class SerialAddress extends SingleByteAddress {
    readonly type: AddressType.Serial = AddressType.Serial as const;

    static serdeable = new TransformSerdeable(
        SingleByteAddress.rawSerdeable,
        (address) => new SerialAddress(address.address()),
        (address) => new SingleByteAddress(address.address()),
    );

    static schema = schema.transform((v) => new SerialAddress(v));

    toString(): string {
        return `${this.type}(${this.address()})`;
    }

    display(): string {
        return `SerialAddress(${this.address()})`;
    }
}
