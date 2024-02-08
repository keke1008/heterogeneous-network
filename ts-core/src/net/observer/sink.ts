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
    NodeSyncFrame,
    ObserverFrame,
    SelfUpdatedEntry,
} from "./frame";
import { P, match } from "ts-pattern";
import {
    DELETE_NETWORK_SUBSCRIPTION_TIMEOUT_MS,
    NETWORK_NOTIFICATION_THROTTLE,
    NOTIFY_NODE_SUBSCRIPTION_INTERVAL_MS,
    REMOVE_UNREACHABLE_NODES_DELAY,
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
import { Throttle, sleep } from "@core/async";
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

class SinkNetworkState {
    #state = new NetworkState();
    #localNodeService: LocalNodeService;

    constructor(args: { localNodeService: LocalNodeService }) {
        this.#localNodeService = args.localNodeService;
    }

    async #removeUnreachableNodes() {
        const localId = await this.#localNodeService.getId();
        const unreachable = this.#state.getUnreachableNodes(localId);
        await sleep(REMOVE_UNREACHABLE_NODES_DELAY);

        const currentUnreachable = this.#state.getUnreachableNodes(localId);
        for (const nodeId of unreachable) {
            if (currentUnreachable.has(nodeId)) {
                this.#state.removeNode(nodeId);
            }
        }
    }

    onReceiveNodeNotificationFrame(source: Source, frame: NodeNotificationFrame): NetworkNotificationEntry[] {
        const partialSource: PartialNode = { nodeId: source.nodeId, clusterId: source.clusterId };
        const result = frame.entries.flatMap((entry: NodeNotificationEntry) => {
            return match(entry)
                .with(P.instanceOf(NeighborUpdatedEntry), (entry) => {
                    return this.#state
                        .updateLink(partialSource, { nodeId: entry.neighbor }, entry.linkCost)
                        .flatMap(NetworkUpdate.intoNotificationEntry);
                })
                .with(P.instanceOf(NeighborRemovedEntry), (entry) => {
                    return this.#state
                        .removeLink(source.nodeId, entry.neighborId)
                        .flatMap(NetworkUpdate.intoNotificationEntry);
                })
                .with(P.instanceOf(SelfUpdatedEntry), (frame) => {
                    return this.#state
                        .updateNode({ ...partialSource, cost: frame.cost })
                        .flatMap(NetworkUpdate.intoNotificationEntry);
                })
                .with(P.instanceOf(FrameReceivedEntry), () => {
                    return [new NetworkFrameReceivedNotificationEntry({ receivedNodeId: source.nodeId })];
                })
                .exhaustive();
        });

        this.#removeUnreachableNodes();
        return result;
    }

    onReceiveNodeSyncFrame(source: Source, frame: NodeSyncFrame) {
        const u1 = this.#state.updateNode({ nodeId: source.nodeId, clusterId: source.clusterId, cost: frame.cost });
        const u2 = this.#state.syncLink(source.nodeId, frame.neighbors);

        this.#removeUnreachableNodes();
        return [...u1, ...u2].flatMap(NetworkUpdate.intoNotificationEntry);
    }

    dumpAsUpdates(): NetworkNotificationEntry[] {
        return this.#state.dumpAsUpdates().flatMap(NetworkUpdate.intoNotificationEntry);
    }
}

export class SinkService {
    #networkState: SinkNetworkState;

    #subscribers = new SubscriberStore();
    #subscriptionSender: NodeSubscriptionSender;

    #notificationSender: NetworkNotificationSender;
    #sendNotificationThrottle = new Throttle<NetworkNotificationEntry>(NETWORK_NOTIFICATION_THROTTLE);

    constructor(args: { socket: RoutingSocket; localNodeService: LocalNodeService; neighborService: NeighborService }) {
        this.#networkState = new SinkNetworkState({ localNodeService: args.localNodeService });
        this.#subscriptionSender = new NodeSubscriptionSender(args);
        this.#subscriptionSender = new NodeSubscriptionSender(args);
        this.#notificationSender = new NetworkNotificationSender(args);
        this.#sendNotificationThrottle.onEmit(async (es) => {
            await this.#notificationSender.send(es, this.#subscribers.getSubscribers());
        });
    }

    dispatchReceivedFrame(source: Source, frame: NetworkSubscriptionFrame | NodeNotificationFrame | NodeSyncFrame) {
        match(frame)
            .with(P.instanceOf(NetworkSubscriptionFrame), (frame) => {
                const isNewSubscriber = this.#subscribers.subscribe(source).isNewSubscriber;
                if (isNewSubscriber || frame.forceDump) {
                    const entries = this.#networkState.dumpAsUpdates();
                    this.#notificationSender.send(entries, [source.intoDestination()]);
                }
            })
            .with(P.instanceOf(NodeNotificationFrame), (frame) => {
                const entries = this.#networkState.onReceiveNodeNotificationFrame(source, frame);
                for (const entry of entries) {
                    this.#sendNotificationThrottle.emit(entry);
                }
            })
            .with(P.instanceOf(NodeSyncFrame), (frame) => {
                const entries = this.#networkState.onReceiveNodeSyncFrame(source, frame);
                for (const entry of entries) {
                    this.#sendNotificationThrottle.emit(entry);
                }
            })
            .exhaustive();
    }

    close() {
        this.#subscriptionSender.close();
    }
}
