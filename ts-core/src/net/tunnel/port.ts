import { z } from "zod";
import { TransformSerdeable, Uint16Serdeable } from "@core/serde";
import { UniqueKey } from "@core/object";

export class TunnelPortId implements UniqueKey {
    #portId: number;

    private constructor(portId: number) {
        this.#portId = portId;
    }

    static readonly schema = z.coerce
        .number()
        .min(0)
        .max(0xff)
        .transform((value) => new TunnelPortId(value));

    static readonly serdeable = new TransformSerdeable(
        new Uint16Serdeable(),
        (value) => new TunnelPortId(value),
        (value) => value.#portId,
    );
    equals(other: TunnelPortId): boolean {
        return this.#portId === other.#portId;
    }

    toString(): string {
        return `TunnelPortId(${this.#portId})`;
    }

    uniqueKey(): number {
        return this.#portId;
    }
}
