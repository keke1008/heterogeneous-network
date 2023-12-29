import { TransformSerdeable, Uint16Serdeable } from "@core/serde";

export class RequestId {
    #value: number;

    constructor(value: number) {
        this.#value = value;
    }

    get(): number {
        return this.#value;
    }

    equals(other: RequestId): boolean {
        return this.#value === other.#value;
    }

    static readonly serdeable = new TransformSerdeable(
        new Uint16Serdeable(),
        (value) => new RequestId(value),
        (id) => id.#value,
    );
}

export class IncrementalRequestIdGenerator {
    #nextId = 0;

    generate(): RequestId {
        return new RequestId(this.#nextId++);
    }
}
