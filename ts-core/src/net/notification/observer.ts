import { FrameIdCache } from "../link";
import { RoutingSocket } from "../routing";
import { MAX_FRAME_ID_CACHE_SIZE, NOTIFY_SUBSCRIBER_TIMEOUT_MS } from "./constants";
import { NotificationFrame, frameToNotification, Notification, SubscriptionFrame, serializeFrame } from "./frame";

export class ObserverService {
    #frameIdCache: FrameIdCache = new FrameIdCache({ maxCacheSize: MAX_FRAME_ID_CACHE_SIZE });
    #onNotify?: (notification: Notification) => void;
    #clearInterval: () => void;

    constructor(routingSocket: RoutingSocket) {
        const emitSubscriptionFrame = () => {
            const frameId = this.#frameIdCache.generate();
            const frame = new SubscriptionFrame({ frameId });
            routingSocket.sendBroadcast(serializeFrame(frame));
        };

        emitSubscriptionFrame();
        const handle = setInterval(emitSubscriptionFrame, NOTIFY_SUBSCRIBER_TIMEOUT_MS);
        this.#clearInterval = () => clearInterval(handle);
    }

    onNotify(onNotify: (notification: Notification) => void) {
        if (this.#onNotify) {
            throw new Error("onNotify already set");
        }
        this.#onNotify = onNotify;
    }

    handleReceiveFrame(frame: NotificationFrame) {
        const notification = frameToNotification(frame);
        this.#onNotify?.(notification);
    }

    onDispose() {
        this.#clearInterval();
    }
}
