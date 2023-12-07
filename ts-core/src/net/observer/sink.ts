import { ObjectMap } from "@core/object";
import { LocalNodeService, NetworkState, NetworkUpdate, NodeId } from "@core/net/node";
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
import { NetworkNotificationFrame, createNetworkNotificationEntryFromNetworkUpdate } from "./frame/network";
import { BufferReader, BufferWriter } from "../buffer";

interface Cancel {
    (): void;
}

class SubscriberStore {
    #subscribers = new ObjectMap<NodeId, Cancel, string>((id) => id.toString());

    subscribe(id: NodeId) {
        this.#subscribers.get(id)?.();

        const timeout = setTimeout(() => {
            this.#subscribers.delete(id);
        }, DELETE_NETWORK_SUBSCRIPTION_TIMEOUT_MS);
        this.#subscribers.set(id, () => clearTimeout(timeout));
    }

    unsubscribe(id: NodeId) {
        this.#subscribers.delete(id);
    }

    getSubscribers(): Iterable<NodeId> {
        return this.#subscribers.keys();
    }
}

class NodeSubscriptionSender {
    #cancel?: Cancel;

    constructor(socket: RoutingSocket) {
        const sendSubscription = async () => {
            const frame = new NodeSubscriptionFrame();
            const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
            frame.serialize(writer);
            const reader = new BufferReader(writer.unwrapBuffer());
            socket.send(NodeId.broadcast(), reader);
        };

        sendSubscription().then(() => {
            const timeout = setInterval(sendSubscription, NOTIFY_NODE_SUBSCRIPTION_INTERVAL_MS);
            this.#cancel = () => clearInterval(timeout);
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

    constructor(args: { socket: RoutingSocket; localNodeService: LocalNodeService }) {
        this.#localNodeService = args.localNodeService;
        this.#socket = args.socket;
        this.#subscriptionSender = new NodeSubscriptionSender(args.socket);
    }

    #sendNetworkNotificationFromUpdate(updates: NetworkUpdate[], destinations: Iterable<NodeId>) {
        const notifications = updates.map(createNetworkNotificationEntryFromNetworkUpdate);
        const networkNotificationFrame = new NetworkNotificationFrame(notifications);
        const writer = new BufferWriter(new Uint8Array(networkNotificationFrame.serializedLength()));
        networkNotificationFrame.serialize(writer);
        const reader = new BufferReader(writer.unwrapBuffer());

        for (const destination of destinations) {
            this.#socket.send(destination, reader);
        }
    }

    dispatchReceivedFrame(sourceId: NodeId, frame: NetworkSubscriptionFrame | NodeNotificationFrame) {
        if (frame instanceof NetworkSubscriptionFrame) {
            this.#subscribers.subscribe(sourceId);
            const updates = this.#networkState.dumpAsUpdates();
            this.#sendNetworkNotificationFromUpdate(updates, [sourceId]);
            return;
        }

        const update = match(frame)
            .with(P.instanceOf(NeighborUpdatedFrame), (frame) => {
                return this.#networkState.updateLink(
                    sourceId,
                    frame.sourceCost,
                    frame.neighborId,
                    frame.neighborCost,
                    frame.linkCost,
                );
            })
            .with(P.instanceOf(NeighborRemovedFrame), (frame) => {
                return this.#networkState.removeLink(sourceId, frame.neighborId);
            })
            .with(P.instanceOf(SelfUpdatedFrame), (frame) => {
                return this.#networkState.updateNode(sourceId, frame.cost);
            })
            .exhaustive();

        const localId = this.#localNodeService.id;
        if (localId !== undefined) {
            update.push(...this.#networkState.removeUnreachableNodes(localId));
        }

        if (update.length > 0) {
            this.#sendNetworkNotificationFromUpdate(update, this.#subscribers.getSubscribers());
        }
    }

    close() {
        this.#subscriptionSender.close();
    }
}
