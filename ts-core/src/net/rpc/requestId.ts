import { UniqueKey } from "@core/object";
import { TransformSerdeable, Uint16Serdeable } from "@core/serde";
import { Keyable } from "@core/types";

export class RequestId implements UniqueKey {
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

    uniqueKey(): Keyable {
        return this.#value;
    }
}

export class IncrementalRequestIdGenerator {
    #nextId = 0;

    generate(): RequestId {
        return new RequestId(this.#nextId++);
    }
}
