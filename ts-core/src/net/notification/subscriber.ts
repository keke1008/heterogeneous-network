import { ObjectMap } from "@core/object";
import { NodeId } from "../routing";

class Subscriber {
    #nodeId: NodeId;
    #timeoutMs: number;
    #remove: () => void;
    #cancel: () => void;

    constructor(nodeId: NodeId, timeoutMs: number, remove: () => void) {
        this.#nodeId = nodeId;
        this.#timeoutMs = timeoutMs;
        this.#remove = remove;

        const timeout = setTimeout(() => this.#remove());
        this.#cancel = () => clearTimeout(timeout);
    }

    nodeId(): NodeId {
        return this.#nodeId;
    }

    touch(): void {
        this.#cancel();
        const timeout = setTimeout(() => this.#remove(), this.#timeoutMs);
        this.#cancel = () => clearTimeout(timeout);
    }
}

export class SubscriberStorage {
    #subscribers: ObjectMap<NodeId, Subscriber, string> = new ObjectMap((nodeId) => nodeId.toString());
    #timeoutMs: number = 1 * 60 * 1000; // 1 minute

    handleReceiveFrame(sender: NodeId) {
        const subscriber = this.#subscribers.get(sender);
        if (subscriber) {
            subscriber.touch();
        } else {
            const remove = () => this.#subscribers.delete(sender);
            this.#subscribers.set(sender, new Subscriber(sender, this.#timeoutMs, remove));
        }
    }

    subscribers(): IterableIterator<NodeId> {
        return this.#subscribers.keys();
    }
}
