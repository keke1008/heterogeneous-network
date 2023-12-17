import { ObjectMap } from "@core/object";
import { Destination, LocalNodeService, NodeId, Source } from "@core/net/node";
import { NotificationService } from "@core/net/notification";
import { consume } from "@core/types";
import { RoutingSocket } from "@core/net/routing";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { NodeSubscriptionFrame, createNodeNotificationFrameFromLocalNotification } from "./frame";
import { DELETE_NODE_SUBSCRIPTION_TIMEOUT_MS } from "./constants";
import { deferred } from "@core/deferred";

interface SubscriberEntry {
    cancel: () => void;
    destination: Destination;
}

class SubscriberStore {
    #subscribers = new ObjectMap<NodeId, SubscriberEntry, string>((id) => id.toString());
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

    constructor(args: {
        localNodeService: LocalNodeService;
        notificationService: NotificationService;
        socket: RoutingSocket;
    }) {
        args.notificationService.onNotification(async (e) => {
            const subscriber = await this.#subscriberStore.getSubscriber();
            const localInfo = await args.localNodeService.getInfo();
            const frame = createNodeNotificationFrameFromLocalNotification(e, localInfo.clusterId, localInfo.cost);
            const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
            frame.serialize(writer);
            const result = await args.socket.send(subscriber, new BufferReader(writer.unwrapBuffer()));
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
