import { ObjectMap } from "@core/object";
import { Destination, NodeId, Source } from "@core/net/node";
import { LocalNotification, NotificationService } from "@core/net/notification";
import { consume } from "@core/types";
import { RoutingSocket } from "@core/net/routing";
import { BufferWriter } from "@core/net/buffer";
import { NodeNotificationFrame, NodeSubscriptionFrame, NodeSyncFrame, ObserverFrame } from "./frame";
import { DELETE_NODE_SUBSCRIPTION_TIMEOUT_MS, NODE_NOTIFICATION_THROTTLE, NODE_SYNC_INTERVAL } from "./constants";
import { Deferred, deferred } from "@core/deferred";
import { Handle, Throttle, sleep, spawn } from "@core/async";
import { LocalNodeService } from "../local";
import { NeighborService } from "../neighbor";

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

    tryGetSubscriber(): Destination | undefined {
        const subscriber = this.#subscribers.entries().next();
        if (subscriber.done) {
            return undefined;
        }

        return subscriber.value[1].destination;
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

class NotifyTask {
    #throttle = new Throttle<LocalNotification>(NODE_NOTIFICATION_THROTTLE);
    #taskHandle: Handle<void>;

    constructor(args: { socket: RoutingSocket; subscriberStore: SubscriberStore }) {
        this.#taskHandle = spawn(async (signal) => {
            while (!signal.aborted) {
                const { destination, onUnsubscribe } = await args.subscriberStore.getSubscriber();
                const cancel = this.#throttle.onEmit(async (es) => {
                    const frame = NodeNotificationFrame.fromLocalNotifications(es);
                    const buffer = BufferWriter.serialize(ObserverFrame.serdeable.serializer(frame)).unwrap();

                    const result = await args.socket.send(destination, buffer);
                    if (result.isErr()) {
                        console.warn("Failed to send node notification", result.unwrapErr());
                    }
                });

                await onUnsubscribe;
                cancel();
            }
        });
    }

    emitNotification(e: LocalNotification) {
        this.#throttle.emit(e);
    }

    async close() {
        this.#taskHandle.cancel();
        await this.#taskHandle;
    }
}

class SyncTask {
    #taskHandle: Handle<void>;

    constructor(args: {
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        socket: RoutingSocket;
        subscriberStore: SubscriberStore;
    }) {
        this.#taskHandle = spawn(async (signal) => {
            while (!signal.aborted) {
                await sleep(NODE_SYNC_INTERVAL);

                const destination = args.subscriberStore.tryGetSubscriber();
                if (destination === undefined) {
                    continue;
                }

                const info = args.localNodeService.tryGetInfo();
                if (info === undefined) {
                    continue;
                }

                const frame = new NodeSyncFrame({
                    source: info.source,
                    cost: info.cost,
                    neighbors: args.neighborService
                        .getNeighborsExceptLocalNode()
                        .map(({ neighbor, linkCost }) => ({ nodeId: neighbor, linkCost })),
                });
                const buffer = BufferWriter.serialize(ObserverFrame.serdeable.serializer(frame)).unwrap();

                const result = await args.socket.send(destination, buffer);
                if (result.isErr()) {
                    console.warn("Failed to send node sync", result.unwrapErr());
                }
            }
        });
    }

    async close() {
        this.#taskHandle.cancel();
        return this.#taskHandle;
    }
}

export class NodeService {
    #subscriberStore = new SubscriberStore();
    #notifyTask: NotifyTask;
    #syncTask: SyncTask;

    constructor(args: {
        notificationService: NotificationService;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        socket: RoutingSocket;
    }) {
        this.#notifyTask = new NotifyTask({ socket: args.socket, subscriberStore: this.#subscriberStore });
        args.notificationService.onNotification((e) => this.#notifyTask.emitNotification(e));

        this.#syncTask = new SyncTask({
            localNodeService: args.localNodeService,
            neighborService: args.neighborService,
            socket: args.socket,
            subscriberStore: this.#subscriberStore,
        });
    }

    dispatchReceivedFrame(source: Source, frame: NodeSubscriptionFrame) {
        consume(frame);
        this.#subscriberStore.subscribe(source);
    }

    async close() {
        await this.#notifyTask.close();
        await this.#syncTask.close();
    }
}
