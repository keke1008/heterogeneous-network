import { TransformSerdeable, Uint8Serdeable } from "@core/serde";
import * as z from "zod";

export const schema = z.union([z.string().min(1), z.number()]).pipe(z.coerce.number().int().min(0).max(255));

export class SingleByteAddress {
    #address: number;

    protected constructor(address: number) {
        this.#address = address;
    }

    static readonly rawSerdeable = new TransformSerdeable(
        new Uint8Serdeable(),
        (address) => new SingleByteAddress(address),
        (address) => address.#address,
    );

    address(): number {
        return this.#address;
    }

    equals(other: SingleByteAddress): boolean {
        return this.#address === other.#address;
    }

    toHumanReadableString(): string {
        return this.#address.toString();
    }
}
