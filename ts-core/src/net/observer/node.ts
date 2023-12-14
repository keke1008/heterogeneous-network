import { ObjectMap } from "@core/object";
import { LocalNodeService, NodeId } from "@core/net/node";
import { NotificationService } from "@core/net/notification";
import { consume } from "@core/types";
import { RoutingSocket } from "@core/net/routing";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { NodeSubscriptionFrame, createNodeNotificationFrameFromLocalNotification } from "./frame";
import { DELETE_NODE_SUBSCRIPTION_TIMEOUT_MS } from "./constants";
import { deferred } from "@core/deferred";

interface Cancel {
    (): void;
}

class SubscriberStore {
    #subscribers = new ObjectMap<NodeId, Cancel, string>((id) => id.toString());
    #waiting? = deferred<NodeId>();

    subscribe(id: NodeId) {
        this.#subscribers.get(id)?.();

        if (this.#waiting !== undefined) {
            this.#waiting.resolve(id);
            this.#waiting = undefined;
        }

        const timeout = setTimeout(() => {
            this.#subscribers.delete(id);
        }, DELETE_NODE_SUBSCRIPTION_TIMEOUT_MS);
        this.#subscribers.set(id, () => clearTimeout(timeout));
    }

    async getSubscriber(): Promise<NodeId> {
        const subscriber = this.#subscribers.entries().next();
        if (!subscriber.done) {
            return subscriber.value[0];
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
            const localCost = await args.localNodeService.getCost();
            const frame = createNodeNotificationFrameFromLocalNotification(e, localCost);
            const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
            frame.serialize(writer);
            const result = await args.socket.send(subscriber, new BufferReader(writer.unwrapBuffer()));
            if (result.isErr()) {
                console.warn("Failed to send node notification", result.unwrapErr());
            }
        });
    }

    dispatchReceivedFrame(sourceId: NodeId, frame: NodeSubscriptionFrame) {
        consume(frame);
        this.#subscriberStore.subscribe(sourceId);
    }
}
