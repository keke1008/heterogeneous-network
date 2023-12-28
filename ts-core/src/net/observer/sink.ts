import { ObjectMap } from "@core/object";
import { Destination, NetworkState, NodeId, Source } from "@core/net/node";
import {
    NeighborRemovedFrame,
    NeighborUpdatedFrame,
    NetworkSubscriptionFrame,
    NodeNotificationFrame,
    NodeSubscriptionFrame,
    SelfUpdatedFrame,
} from "./frame";
import { P, match } from "ts-pattern";
import { DELETE_NETWORK_SUBSCRIPTION_TIMEOUT_MS, NOTIFY_NODE_SUBSCRIPTION_INTERVAL_MS } from "./constants";
import { RoutingSocket } from "../routing";
import {
    NetworkFrameReceivedNotificationEntry,
    NetworkNotificationEntry,
    NetworkNotificationFrame,
    NetworkUpdate,
} from "./frame/network";
import { BufferReader, BufferWriter } from "../buffer";
import { NeighborService } from "../neighbor";
import { LocalNodeService } from "../local";
import { FrameReceivedFrame } from "./frame/node";

interface SubscriberEntry {
    cancel: () => void;
    destination: Destination;
}

class SubscriberStore {
    #subscribers = new ObjectMap<NodeId, SubscriberEntry, string>((id) => id.toString());

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
            const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
            frame.serialize(writer);
            const reader = new BufferReader(writer.unwrapBuffer());
            socket.send(destination, reader);
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

export class SinkService {
    #localNodeService: LocalNodeService;
    #subscribers = new SubscriberStore();
    #networkState = new NetworkState();
    #socket: RoutingSocket;
    #subscriptionSender: NodeSubscriptionSender;

    constructor(args: { socket: RoutingSocket; neighborService: NeighborService; localNodeService: LocalNodeService }) {
        this.#localNodeService = args.localNodeService;
        this.#socket = args.socket;
        this.#subscriptionSender = new NodeSubscriptionSender(args);
    }

    #sendNetworkNotificationFromUpdate(entries: NetworkNotificationEntry[], destinations: Iterable<Destination>) {
        if (entries.length === 0) {
            return;
        }
        const networkNotificationFrame = new NetworkNotificationFrame(entries);
        const writer = new BufferWriter(new Uint8Array(networkNotificationFrame.serializedLength()));
        networkNotificationFrame.serialize(writer);
        const reader = new BufferReader(writer.unwrapBuffer());

        for (const destination of destinations) {
            this.#socket.send(destination, reader.initialized());
        }
    }

    dispatchReceivedFrame(source: Source, frame: NetworkSubscriptionFrame | NodeNotificationFrame) {
        if (frame instanceof NetworkSubscriptionFrame) {
            if (this.#subscribers.subscribe(source).isNewSubscriber) {
                const entries = this.#networkState.dumpAsUpdates().map(NetworkUpdate.intoNotificationEntry);
                this.#sendNetworkNotificationFromUpdate(entries, [source.intoDestination()]);
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

        this.#sendNetworkNotificationFromUpdate(update, this.#subscribers.getSubscribers());
    }

    close() {
        this.#subscriptionSender.close();
    }
}
