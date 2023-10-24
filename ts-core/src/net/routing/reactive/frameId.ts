import { BufferReader, BufferWriter } from "../../buffer";

export class FrameId {
    id: number;

    constructor(id: number) {
        this.id = id;
    }

    equals(other: FrameId): boolean {
        return this.id === other.id;
    }

    static deserialize(reader: BufferReader): FrameId {
        return new FrameId(reader.readByte());
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.id);
    }

    serializedLength(): number {
        return 1;
    }
}

export class FrameIdManager {
    #cache: number[] = [];
    #maxCacheLength: number;

    constructor(option?: { maxCacheLength?: number }) {
        this.#maxCacheLength = option?.maxCacheLength ?? 8;
    }

    #push(frameId: FrameId): void {
        this.#cache.push(frameId.id);
        if (this.#cache.length > this.#maxCacheLength) {
            this.#cache.shift();
        }
    }

    onReceived(frameId: FrameId): "discard" | "accept" {
        if (this.#cache.includes(frameId.id)) {
            return "discard";
        }

        this.#push(frameId);
        return "accept";
    }

    next(): FrameId {
        let id: FrameId;
        do {
            id = new FrameId(Math.floor(Math.random() * 256));
        } while (this.#cache.includes(id.id));

        this.#push(id);
        return id;
    }
}
