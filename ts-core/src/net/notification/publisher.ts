import { BufferReader, BufferWriter } from "../buffer";
import { NodeId, RoutingSocket } from "../routing";
import { NotificationFrame, notificationToFrame, Notification } from "./frame";
import { SubscriberStorage } from "./subscriber";

export class PublisherService {
    #routingSocket: RoutingSocket;
    #subscriber: SubscriberStorage = new SubscriberStorage();
    #localSubscriber?: (frame: NotificationFrame) => void;

    constructor(routingSocket: RoutingSocket) {
        this.#routingSocket = routingSocket;
    }

    subscribeLocal(onNotify: (frame: NotificationFrame) => void) {
        if (this.#localSubscriber) {
            throw new Error("local subscriber already set");
        }
        this.#localSubscriber = onNotify;
    }

    handleReceiveFrame(sender: NodeId) {
        this.#subscriber.handleReceiveFrame(sender);
    }

    #sendNotification(frame: NotificationFrame): void {
        this.#localSubscriber?.(frame);
        const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
        frame.serialize(writer);
        const buffer = writer.unwrapBuffer();

        for (const nodeId of this.#subscriber.subscribers()) {
            this.#routingSocket.send(nodeId, new BufferReader(buffer));
        }
    }

    publishNotification(notification: Notification) {
        const frame = notificationToFrame(notification);
        this.#sendNotification(frame);
    }
}
