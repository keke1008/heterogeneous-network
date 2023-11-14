import { NotificationService } from "../notification";
import { FrameIdCache, LinkService, Protocol } from "../link";
import { ReactiveService, RoutingSocket } from "../routing";
import { PublishBroker } from "./broker";
import { SubscribeFrame, deserializeObserverFrame, fromNotification } from "./frame";
import { MAX_FRAME_ID_CACHE_SIZE } from "./constants";
import { LocalNotificationSubscriber, SubscribeManager } from "./subscribe";

export class ObserverService {
    #frameIdCache: FrameIdCache = new FrameIdCache({ maxCacheSize: MAX_FRAME_ID_CACHE_SIZE });
    #broker: PublishBroker;
    #subscribeManager: SubscribeManager;

    constructor(args: {
        linkService: LinkService;
        reactiveService: ReactiveService;
        notificationService: NotificationService;
    }) {
        const linkSocket = args.linkService.open(Protocol.Observer);
        const socket = new RoutingSocket(linkSocket, args.reactiveService);
        this.#broker = new PublishBroker(socket);
        this.#subscribeManager = new SubscribeManager(socket);

        args.notificationService.onNotification((notification) => {
            this.#subscribeManager.dispatchNotification(notification);

            const frame = fromNotification(notification);
            this.#broker.publish(frame);
        });

        socket.onReceive((frame) => {
            const received = deserializeObserverFrame(frame.reader);
            if (received.isErr()) {
                return;
            }

            const observerFrame = received.unwrap();
            if (observerFrame instanceof SubscribeFrame) {
                if (!this.#frameIdCache.has(observerFrame.frameId)) {
                    this.#frameIdCache.add(observerFrame.frameId);
                    this.#broker.acceptSubscription(frame.senderId);
                }
            } else {
                this.#subscribeManager.dispatchNotification(observerFrame.intoNotification());
            }
        });
    }

    requestSubscribe(subscriber: LocalNotificationSubscriber) {
        this.#subscribeManager.addLocalSubscriber(subscriber, this.#frameIdCache);
    }

    dispose() {
        this.#subscribeManager.dispose();
    }
}
