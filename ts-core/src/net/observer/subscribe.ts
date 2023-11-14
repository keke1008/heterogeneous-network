import { BufferReader, BufferWriter } from "../buffer";
import { FrameIdCache } from "../link";
import { NetNotification } from "../notification";
import { RoutingSocket } from "../routing";
import { SubscribeFrame } from "./frame";
import { NOTIFY_SUBSCRIBER_INTERVAL_MS } from "./constants";

export interface LocalNotificationSubscriber {
    onNotification(notification: NetNotification): void;
}

class SubscribeRequester {
    #socket: RoutingSocket;
    #frameIdCache: FrameIdCache;
    #onDispose: () => void;

    constructor(socket: RoutingSocket, frameIdCache: FrameIdCache) {
        this.#socket = socket;
        this.#frameIdCache = frameIdCache;

        this.#sendSubscribeRequest();
        const interval = setInterval(() => this.#sendSubscribeRequest(), NOTIFY_SUBSCRIBER_INTERVAL_MS);
        this.#onDispose = () => clearInterval(interval);
    }

    #sendSubscribeRequest() {
        const frame = new SubscribeFrame({ frameId: this.#frameIdCache.generate() });
        const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
        frame.serialize(writer);

        this.#socket.sendBroadcast(new BufferReader(writer.unwrapBuffer()));
    }

    dispose() {
        this.#onDispose();
    }
}

export class SubscribeManager {
    #localSubscribers: LocalNotificationSubscriber[] = [];
    #subscribeRequester?: SubscribeRequester;
    #socket: RoutingSocket;

    constructor(socket: RoutingSocket) {
        this.#socket = socket;
    }

    addLocalSubscriber(subscriber: LocalNotificationSubscriber, frameIdCache: FrameIdCache) {
        if (this.#localSubscribers.length === 0) {
            this.#subscribeRequester = new SubscribeRequester(this.#socket, frameIdCache);
        }
        this.#localSubscribers.push(subscriber);
    }

    dispatchNotification(notification: NetNotification): void {
        for (const subscriber of this.#localSubscribers) {
            subscriber.onNotification(notification);
        }
    }

    dispose() {
        this.#subscribeRequester?.dispose();
    }
}
