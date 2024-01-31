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
        .max(0xffff)
        .transform((value) => new TunnelPortId(value));

    static readonly serdeable = new TransformSerdeable(
        new Uint16Serdeable(),
        (value) => new TunnelPortId(value),
        (value) => value.#portId,
    );

    static serializedLength(): number {
        return this.serdeable.serializer(new TunnelPortId(0)).serializedLength();
    }

    static generateRandomDynamicPort(): TunnelPortId {
        const min = 1024;
        const max = 65535;
        const range = max - min;
        return new TunnelPortId(Math.floor(Math.random() * range) + min);
    }

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
