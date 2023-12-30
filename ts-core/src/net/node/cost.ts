import { Err, Ok, Result } from "oxide.ts";
import { TransformSerdeable, Uint16Serdeable } from "@core/serde";
import * as z from "zod";
import { Duration } from "@core/time";

export class Cost {
    #cost: number;

    constructor(cost: number) {
        if (cost < 0 || cost > 0xffff) {
            throw new Error(`Cost must be between 0 and 65535, but was ${cost}`);
        }
        this.#cost = Math.floor(cost);
    }

    static fromNumber(cost: number): Result<Cost, void> {
        if (cost < 0 || cost > 0xffff) {
            return Err(undefined);
        }
        return Ok(new Cost(cost));
    }

    static readonly serdeable = new TransformSerdeable(
        new Uint16Serdeable(),
        (cost) => new Cost(cost),
        (cost) => cost.#cost,
    );

    static schema = z
        .union([z.number(), z.string().min(1)])
        .pipe(z.coerce.number().int().min(0).max(0xffff))
        .transform((value) => new Cost(value));

    get(): number {
        return this.#cost;
    }

    equals(other: Cost): boolean {
        return this.#cost === other.#cost;
    }

    add(other: Cost): Cost {
        return new Cost(this.#cost + other.#cost);
    }

    lessThan(other: Cost): boolean {
        return this.#cost < other.#cost;
    }

    toString(): string {
        return `Cost(${this.#cost})`;
    }

    intoDuration(): Duration {
        return Duration.fromMillies(this.#cost);
    }
}
