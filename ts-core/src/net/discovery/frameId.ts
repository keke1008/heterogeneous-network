import { TransformSerdeable, Uint16Serdeable } from "@core/serde";

export class FrameId {
    #id: number;

    constructor(id: number) {
        this.#id = id;
    }

    static random(): FrameId {
        return new FrameId(Math.floor(Math.random() * 65536));
    }

    get(): number {
        return this.#id;
    }

    equals(other: FrameId): boolean {
        return this.#id === other.#id;
    }

    static readonly serdeable = new TransformSerdeable(
        new Uint16Serdeable(),
        (id) => new FrameId(id),
        (id) => id.#id,
    );

    display(): string {
        return `FrameId(${this.#id})`;
    }
}

export class FrameIdCache {
    #cache: Set<number> = new Set();
    #maxCacheSize: number;

    constructor(opts: { maxCacheSize?: number }) {
        this.#maxCacheSize = opts?.maxCacheSize ?? 10;
    }

    insert(id: FrameId): void {
        this.#cache.add(id.get());
        if (this.#cache.size > this.#maxCacheSize) {
            const first = this.#cache.values().next().value;
            this.#cache.delete(first);
        }
    }

    generateWithoutAdding(): FrameId {
        let id;
        do {
            id = FrameId.random();
        } while (this.#cache.has(id.get()));

        return id;
    }

    generate(): FrameId {
        const id = this.generateWithoutAdding();
        this.insert(id);
        return id;
    }

    insertAndCheckContains(id: FrameId): boolean {
        const contains = this.#cache.has(id.get());
        this.insert(id);
        return contains;
    }
}
