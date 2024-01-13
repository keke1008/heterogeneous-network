import { ObjectMap } from "@core/object";
import { Destination, NodeId, Source } from "@core/net/node";
import { NotificationService } from "@core/net/notification";
import { consume } from "@core/types";
import { RoutingSocket } from "@core/net/routing";
import { BufferWriter } from "@core/net/buffer";
import { NodeSubscriptionFrame, ObserverFrame, createNodeNotificationFrameFromLocalNotification } from "./frame";
import { DELETE_NODE_SUBSCRIPTION_TIMEOUT_MS } from "./constants";
import { deferred } from "@core/deferred";

interface SubscriberEntry {
    cancel: () => void;
    destination: Destination;
}

class SubscriberStore {
    #subscribers = new ObjectMap<NodeId, SubscriberEntry>();
    #waiting? = deferred<Destination>();

    subscribe(subscriber: Source) {
        this.#subscribers.get(subscriber.nodeId)?.cancel();

        if (this.#waiting !== undefined) {
            this.#waiting.resolve(subscriber.intoDestination());
            this.#waiting = undefined;
        }

        const timeout = setTimeout(() => {
            this.#subscribers.delete(subscriber.nodeId);
        }, DELETE_NODE_SUBSCRIPTION_TIMEOUT_MS);
        this.#subscribers.set(subscriber.nodeId, {
            cancel: () => clearTimeout(timeout),
            destination: subscriber.intoDestination(),
        });
    }

    async getSubscriber(): Promise<Destination> {
        const subscriber = this.#subscribers.entries().next();
        if (!subscriber.done) {
            return subscriber.value[1].destination;
        }

        this.#waiting ??= deferred();
        return this.#waiting;
    }
}

export class NodeService {
    #subscriberStore = new SubscriberStore();

    constructor(args: { notificationService: NotificationService; socket: RoutingSocket }) {
        args.notificationService.onNotification(async (e) => {
            const subscriber = await this.#subscriberStore.getSubscriber();
            const frame = createNodeNotificationFrameFromLocalNotification(e);

            const buffer = BufferWriter.serialize(ObserverFrame.serdeable.serializer(frame)).expect(
                "Failed to serialize frame",
            );
            const result = await args.socket.send(subscriber, buffer);
            if (result.isErr()) {
                console.warn("Failed to send node notification", result.unwrapErr());
            }
        });
    }

    dispatchReceivedFrame(source: Source, frame: NodeSubscriptionFrame) {
        consume(frame);
        this.#subscriberStore.subscribe(source);
    }
}
