import { ReactiveService, RoutingFrame, RoutingSocket } from "../routing";
import { ObserverService } from "./observer";
import { PublisherService } from "./publisher";
import { FrameType, Notification, deserializeFrame } from "./frame";
import { LinkService, Protocol } from "../link";

export class NotificationService {
    #routingSocket: RoutingSocket;
    #publisher: PublisherService;
    #observer?: ObserverService;

    constructor(args: { linkService: LinkService; reactiveService: ReactiveService }) {
        const socket = args.linkService.open(Protocol.Notification);
        this.#routingSocket = new RoutingSocket(socket, args.reactiveService);
        this.#publisher = new PublisherService(this.#routingSocket);
        this.#routingSocket.onReceive((frame) => this.#handleReceiveFrame(frame));
    }

    #handleReceiveFrame(frame: RoutingFrame) {
        const notificationFrame = deserializeFrame(frame.reader);
        if (notificationFrame.type === FrameType.Subscription) {
            this.#publisher.handleReceiveFrame(frame.senderId);
        } else {
            this.#observer?.handleReceiveFrame(notificationFrame);
        }
    }

    subscribe(onNotify: (notification: Notification) => void) {
        if (this.#observer) {
            throw new Error("observer already set");
        }

        this.#observer = new ObserverService(this.#routingSocket);
        this.#publisher.subscribeLocal((frame) => this.#observer?.handleReceiveFrame(frame));
        this.#observer.onNotify(onNotify);
    }

    onDispose() {
        this.#observer?.onDispose();
    }
}
