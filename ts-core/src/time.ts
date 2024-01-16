export class Duration {
    #ms: number;

    private constructor(ms: number) {
        this.#ms = ms;
    }

    get millies(): number {
        return this.#ms;
    }

    get seconds(): number {
        return this.#ms / 1000;
    }

    static fromMillies(ms: number): Duration {
        return new Duration(ms);
    }

    static fromSeconds(seconds: number): Duration {
        return new Duration(seconds * 1000);
    }

    add(other: Duration): Duration {
        return new Duration(this.#ms + other.#ms);
    }

    subtract(other: Duration): Duration {
        return new Duration(this.#ms - other.#ms);
    }

    multiply(factor: number): Duration {
        return new Duration(this.#ms * factor);
    }

    divide(factor: number): Duration {
        return new Duration(this.#ms / factor);
    }

    lessThan(other: Duration): boolean {
        return this.#ms < other.#ms;
    }
}

export class Instant {
    #ms: number;

    private constructor(ms: number) {
        this.#ms = ms;
    }

    static now(): Instant {
        return new Instant(Date.now());
    }

    subtract(other: Instant): Duration {
        return Duration.fromMillies(this.#ms - other.#ms);
    }

    add(duration: Duration): Instant {
        return new Instant(this.#ms + duration.millies);
    }

    elapsed(): Duration {
        return Instant.now().subtract(this);
    }
}
