import { BufferReader, BufferWriter } from "../buffer";

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

    static deserialize(reader: BufferReader): FrameId {
        return new FrameId(reader.readUint16());
    }

    serialize(writer: BufferWriter) {
        writer.writeUint16(this.#id);
    }

    serializedLength(): number {
        return 2;
    }
}

export class FrameIdCache {
    #cache: Set<number> = new Set();
    #maxCacheSize: number;

    constructor(opts: { maxCacheSize?: number }) {
        this.#maxCacheSize = opts?.maxCacheSize ?? 10;
    }

    add(id: FrameId): void {
        this.#cache.add(id.get());
        if (this.#cache.size > this.#maxCacheSize) {
            const first = this.#cache.values().next().value;
            this.#cache.delete(first);
        }
    }

    generate(): FrameId {
        let id;
        do {
            id = FrameId.random();
        } while (this.#cache.has(id.get()));

        this.add(id);
        return id;
    }

    has(id: FrameId): boolean {
        return this.#cache.has(id.get());
    }
}

export class IncrementalFrameIdGenerator {
    #nextId: number = 0;

    generate(): FrameId {
        const id = new FrameId(this.#nextId);
        this.#nextId = (this.#nextId + 1) % 65536;
        return id;
    }
}
