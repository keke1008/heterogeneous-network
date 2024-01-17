import { TransformSerdeable, Uint16Serdeable } from "@core/serde";

export class SequenceNumber {
    #value: number;

    private constructor(value: number) {
        this.#value = value;
    }

    static zero(): SequenceNumber {
        return new SequenceNumber(0);
    }

    next(): SequenceNumber {
        return new SequenceNumber(this.#value + 1);
    }

    equals(other: SequenceNumber): boolean {
        return this.#value === other.#value;
    }

    isOlderThan(other: SequenceNumber): boolean {
        return this.#value < other.#value;
    }

    static readonly serdeable = new TransformSerdeable(
        new Uint16Serdeable(),
        (value) => new SequenceNumber(value),
        (value) => value.#value,
    );
}
