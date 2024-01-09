import { ObjectMap } from "@core/object";
import { Destination, NetworkState, NodeId, Source } from "@core/net/node";
import {
    NeighborRemovedFrame,
    NeighborUpdatedFrame,
    NetworkSubscriptionFrame,
    NodeNotificationFrame,
    NodeSubscriptionFrame,
    ObserverFrame,
    SelfUpdatedFrame,
} from "./frame";
import { P, match } from "ts-pattern";
import {
    DELETE_NETWORK_SUBSCRIPTION_TIMEOUT_MS,
    NETWORK_NOTIFICATION_THROTTLE,
    NOTIFY_NODE_SUBSCRIPTION_INTERVAL_MS,
} from "./constants";
import { RoutingSocket } from "../routing";
import {
    NetworkFrameReceivedNotificationEntry,
    NetworkNotificationEntry,
    NetworkNotificationFrame,
    NetworkUpdate,
} from "./frame/network";
import { BufferWriter } from "../buffer";
import { NeighborService } from "../neighbor";
import { LocalNodeService } from "../local";
import { FrameReceivedFrame } from "./frame/node";
import { Handle, sleep, spawn } from "@core/async";

interface SubscriberEntry {
    cancel: () => void;
    destination: Destination;
}

class SubscriberStore {
    #subscribers = new ObjectMap<NodeId, SubscriberEntry>();

    subscribe(subscriber: Source) {
        const old = this.#subscribers.get(subscriber.nodeId);
        old?.cancel();

        const timeout = setTimeout(() => {
            this.#subscribers.delete(subscriber.nodeId);
        }, DELETE_NETWORK_SUBSCRIPTION_TIMEOUT_MS);
        this.#subscribers.set(subscriber.nodeId, {
            cancel: () => clearTimeout(timeout),
            destination: subscriber.intoDestination(),
        });

        return { isNewSubscriber: old === undefined };
    }

    unsubscribe(id: NodeId) {
        this.#subscribers.delete(id);
    }

    getSubscribers(): Destination[] {
        return [...this.#subscribers.values()].map((entry) => entry.destination);
    }
}

class NodeSubscriptionSender {
    #cancel?: () => void;

    constructor(args: { socket: RoutingSocket; neighborService: NeighborService }) {
        const { socket, neighborService } = args;

        const sendSubscription = async (destination = Destination.broadcast()) => {
            const frame = new NodeSubscriptionFrame();
            const buffer = BufferWriter.serialize(ObserverFrame.serdeable.serializer(frame)).expect(
                "Failed to serialize frame",
            );
            socket.send(destination, buffer);
        };

        sendSubscription().then(() => {
            const timeout = setInterval(sendSubscription, NOTIFY_NODE_SUBSCRIPTION_INTERVAL_MS);
            this.#cancel = () => clearInterval(timeout);
        });

        neighborService.onNeighborAdded((neighbor) => {
            sendSubscription(neighbor.neighbor.intoDestination());
        });
    }

    close() {
        this.#cancel?.();
    }
}

class NetworkNotificationSender {
    #socket: RoutingSocket;

    constructor(args: { socket: RoutingSocket }) {
        this.#socket = args.socket;
    }

    async send(entries: NetworkNotificationEntry[], destinations: Iterable<Destination>) {
        if (entries.length === 0) {
            return;
        }

        const frame = new NetworkNotificationFrame(entries);
        const buffer = BufferWriter.serialize(ObserverFrame.serdeable.serializer(frame)).unwrap();
        for await (const destination of destinations) {
            await this.#socket.send(destination, buffer);
        }
    }
}

class ThrottledNotificationSender {
    #buffer: NetworkNotificationEntry[] = [];
    #subscriberStore: SubscriberStore;
    #sender: NetworkNotificationSender;
    #timer?: Handle<void>;

    constructor(args: { subscriberStore: SubscriberStore; sender: NetworkNotificationSender }) {
        this.#subscriberStore = args.subscriberStore;
        this.#sender = args.sender;
    }

    add(entries: NetworkNotificationEntry[]) {
        this.#buffer.push(...entries);
        if (this.#timer !== undefined) {
            return;
        }

        this.#timer = spawn(async () => {
            await sleep(NETWORK_NOTIFICATION_THROTTLE);
            await this.#sender.send(this.#buffer, this.#subscriberStore.getSubscribers());
            this.#buffer = [];
        });
    }
}

export class SinkService {
    #localNodeService: LocalNodeService;
    #networkState = new NetworkState();

    #subscribers = new SubscriberStore();
    #subscriptionSender: NodeSubscriptionSender;

    #notificationSender: NetworkNotificationSender;
    #throttledNotificationSender: ThrottledNotificationSender;

    constructor(args: { socket: RoutingSocket; neighborService: NeighborService; localNodeService: LocalNodeService }) {
        this.#localNodeService = args.localNodeService;
        this.#subscriptionSender = new NodeSubscriptionSender(args);
        this.#subscriptionSender = new NodeSubscriptionSender(args);
        this.#notificationSender = new NetworkNotificationSender(args);
        this.#throttledNotificationSender = new ThrottledNotificationSender({
            subscriberStore: this.#subscribers,
            sender: this.#notificationSender,
        });
    }

    dispatchReceivedFrame(source: Source, frame: NetworkSubscriptionFrame | NodeNotificationFrame) {
        if (frame instanceof NetworkSubscriptionFrame) {
            if (this.#subscribers.subscribe(source).isNewSubscriber) {
                const entries = this.#networkState.dumpAsUpdates().map(NetworkUpdate.intoNotificationEntry);
                this.#notificationSender.send(entries, [source.intoDestination()]);
            }
            return;
        }

        const update = match(frame)
            .with(P.instanceOf(NeighborUpdatedFrame), (frame) => {
                return this.#networkState
                    .updateLink(source, frame.localCost, frame.neighbor, frame.neighborCost, frame.linkCost)
                    .map(NetworkUpdate.intoNotificationEntry);
            })
            .with(P.instanceOf(NeighborRemovedFrame), (frame) => {
                return this.#networkState
                    .removeLink(source.nodeId, frame.neighborId)
                    .map(NetworkUpdate.intoNotificationEntry);
            })
            .with(P.instanceOf(SelfUpdatedFrame), (frame) => {
                return this.#networkState.updateNode(source, frame.cost).map(NetworkUpdate.intoNotificationEntry);
            })
            .with(P.instanceOf(FrameReceivedFrame), () => {
                return [new NetworkFrameReceivedNotificationEntry({ receivedNodeId: source.nodeId })];
            })
            .exhaustive();

        const localId = this.#localNodeService.id;
        if (localId !== undefined) {
            update.push(...this.#networkState.removeUnreachableNodes(localId).map(NetworkUpdate.intoNotificationEntry));
        }

        this.#throttledNotificationSender.add(update);
    }

    close() {
        this.#subscriptionSender.close();
    }
}
