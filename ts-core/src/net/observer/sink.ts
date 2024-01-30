import { ObjectMap } from "@core/object";
import { Destination, NetworkState, NodeId, PartialNode, Source } from "@core/net/node";
import {
    FrameReceivedEntry,
    NeighborRemovedEntry,
    NeighborUpdatedEntry,
    NetworkSubscriptionFrame,
    NodeNotificationEntry,
    NodeNotificationFrame,
    NodeSubscriptionFrame,
    ObserverFrame,
    SelfUpdatedEntry,
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
import { Throttle } from "@core/async";
import { LocalNodeService } from "../local";

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
            sendSubscription(Destination.fromNodeId(neighbor.neighbor));
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

export class SinkService {
    #networkState = new NetworkState();

    #subscribers = new SubscriberStore();
    #subscriptionSender: NodeSubscriptionSender;

    #notificationSender: NetworkNotificationSender;
    #sendNotificationThrottle = new Throttle<NetworkNotificationEntry>(NETWORK_NOTIFICATION_THROTTLE);

    constructor(args: { socket: RoutingSocket; localNodeService: LocalNodeService; neighborService: NeighborService }) {
        this.#subscriptionSender = new NodeSubscriptionSender(args);
        this.#subscriptionSender = new NodeSubscriptionSender(args);
        this.#notificationSender = new NetworkNotificationSender(args);
        this.#sendNotificationThrottle.onEmit(async (es) => {
            await this.#notificationSender.send(es, this.#subscribers.getSubscribers());
        });

        args.localNodeService.getInfo().then((info) => {
            this.#networkState.updateNode({ nodeId: info.id, cost: info.cost, clusterId: info.clusterId });
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

        const partialSource: PartialNode = { nodeId: source.nodeId, clusterId: source.clusterId };

        const applyUpdate = (entry: NodeNotificationEntry) => {
            return match(entry)
                .with(P.instanceOf(NeighborUpdatedEntry), (entry) => {
                    return this.#networkState
                        .updateLink(partialSource, { nodeId: entry.neighbor }, entry.linkCost)
                        .map(NetworkUpdate.intoNotificationEntry);
                })
                .with(P.instanceOf(NeighborRemovedEntry), (entry) => {
                    return this.#networkState
                        .removeLink(source.nodeId, entry.neighborId)
                        .map(NetworkUpdate.intoNotificationEntry);
                })
                .with(P.instanceOf(SelfUpdatedEntry), (frame) => {
                    return this.#networkState
                        .updateNode({ ...partialSource, cost: frame.cost })
                        .map(NetworkUpdate.intoNotificationEntry);
                })
                .with(P.instanceOf(FrameReceivedEntry), () => {
                    return [new NetworkFrameReceivedNotificationEntry({ receivedNodeId: source.nodeId })];
                })
                .exhaustive();
        };

        for (const entry of frame.entries.flatMap(applyUpdate)) {
            this.#sendNotificationThrottle.emit(entry);
        }
    }

    close() {
        this.#subscriptionSender.close();
    }
}
