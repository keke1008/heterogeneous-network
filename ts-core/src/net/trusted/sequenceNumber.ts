import { TransformSerdeable, Uint16Serdeable } from "@core/serde";

export class SequenceNumber {
    #value: number;

    constructor(value: number) {
        this.#value = value;
    }

    static zero(): SequenceNumber {
        return new SequenceNumber(0);
    }

    static fromAck(ack: AcknowledgementNumber): SequenceNumber {
        return new SequenceNumber(ack.value());
    }

    value(): number {
        return this.#value;
    }

    static readonly serdeable = new TransformSerdeable(
        new Uint16Serdeable(),
        (value) => new SequenceNumber(value),
        (value) => value.#value,
    );
}

export class AcknowledgementNumber {
    #value: number;

    constructor(value: number) {
        this.#value = value;
    }

    static zero(): AcknowledgementNumber {
        return new AcknowledgementNumber(0);
    }

    value(): number {
        return this.#value;
    }

    static readonly serdeable = new TransformSerdeable(
        new Uint16Serdeable(),
        (value) => new AcknowledgementNumber(value),
        (value) => value.#value,
    );
}
