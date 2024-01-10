import { FrameId, FrameIdCache } from "../discovery";

export class AsymmetricalFrameIdCache {
    #send: FrameIdCache;
    #receive: FrameIdCache;

    constructor(args: { maxCacheSize: number }) {
        this.#send = new FrameIdCache({ maxCacheSize: args.maxCacheSize });
        this.#receive = new FrameIdCache({ maxCacheSize: args.maxCacheSize });
    }

    generate(opts: { loopbackable: boolean }): FrameId {
        let id;
        do {
            id = this.#send.generate();
        } while (this.#receive.contains(id));

        this.#send.insert(id);
        opts.loopbackable || this.#receive.insert(id);
        return id;
    }

    insertAndCheckContainsAsReceived(id: FrameId): boolean {
        this.#send.insertAndCheckContains(id);
        const receiveContains = this.#receive.insertAndCheckContains(id);
        return receiveContains;
    }

    containsAsSent(id: FrameId): boolean {
        return this.#send.contains(id);
    }
}
