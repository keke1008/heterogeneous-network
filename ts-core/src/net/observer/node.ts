import { ObjectMap } from "@core/object";
import { Destination, NodeId, Source } from "@core/net/node";
import { LocalNotification, NotificationService } from "@core/net/notification";
import { consume } from "@core/types";
import { RoutingSocket } from "@core/net/routing";
import { BufferWriter } from "@core/net/buffer";
import { NodeNotificationFrame, NodeSubscriptionFrame, ObserverFrame } from "./frame";
import { DELETE_NODE_SUBSCRIPTION_TIMEOUT_MS, NODE_NOTIFICATION_THROTTLE } from "./constants";
import { Deferred, deferred } from "@core/deferred";
import { Throttle } from "@core/async";

interface SubscriberEntry {
    cancel: () => void;
    destination: Destination;
    onUnsubscribe: Deferred<void>;
}

class SubscriberStore {
    #subscribers = new ObjectMap<NodeId, SubscriberEntry>();
    #waiting? = deferred<SubscriberEntry>();

    subscribe(subscriber: Source) {
        this.#subscribers.get(subscriber.nodeId)?.cancel();

        const timeout = setTimeout(() => {
            this.#subscribers.get(subscriber.nodeId)?.onUnsubscribe.resolve();
            this.#subscribers.delete(subscriber.nodeId);
        }, DELETE_NODE_SUBSCRIPTION_TIMEOUT_MS);
        const entry = {
            cancel: () => clearTimeout(timeout),
            destination: subscriber.intoDestination(),
            onUnsubscribe: deferred<void>(),
        };

        this.#subscribers.set(subscriber.nodeId, entry);
        if (this.#waiting !== undefined) {
            this.#waiting.resolve(entry);
            this.#waiting = undefined;
        }
    }

    async getSubscriber(): Promise<{ destination: Destination; onUnsubscribe: Promise<void> }> {
        const subscriber = this.#subscribers.entries().next();
        if (!subscriber.done) {
            return subscriber.value[1];
        }

        this.#waiting ??= deferred();
        return this.#waiting;
    }
}

export class NodeService {
    #subscriberStore = new SubscriberStore();
    #throttle = new Throttle<LocalNotification>(NODE_NOTIFICATION_THROTTLE);

    constructor(args: { notificationService: NotificationService; socket: RoutingSocket }) {
        args.notificationService.onNotification(async (e) => this.#throttle.emit(e));

        this.#subscriberStore.getSubscriber().then(({ destination, onUnsubscribe }) => {
            const cancel = this.#throttle.onEmit(async (es) => {
                const frame = NodeNotificationFrame.fromLocalNotifications(es);
                const buffer = BufferWriter.serialize(ObserverFrame.serdeable.serializer(frame)).unwrap();

                const result = await args.socket.send(destination, buffer);
                if (result.isErr()) {
                    console.warn("Failed to send node notification", result.unwrapErr());
                }
            });

            onUnsubscribe.then(() => cancel());
        });
    }

    dispatchReceivedFrame(source: Source, frame: NodeSubscriptionFrame) {
        consume(frame);
        this.#subscriberStore.subscribe(source);
    }
}
