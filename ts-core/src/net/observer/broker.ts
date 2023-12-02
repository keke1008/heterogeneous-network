import { ObjectMap } from "@core/object";
import { NodeId } from "@core/net/node";
import { RoutingSocket } from "../routing";
import { DELETE_SUBSCRIBER_TIMEOUT_MS } from "./constants";
import { NotifyFrame } from "./frame";
import { BufferReader, BufferWriter } from "../buffer";

interface Subscriber {
    nodeId: NodeId;
    removeTimeout: () => void;
}

export class PublishBroker {
    #subscribers: ObjectMap<NodeId, Subscriber, string> = new ObjectMap((id) => id.toString());
    #socket: RoutingSocket;

    constructor(socket: RoutingSocket) {
        this.#socket = socket;
    }

    acceptSubscription(nodeId: NodeId): void {
        const subscriber = this.#subscribers.get(nodeId);
        if (subscriber !== undefined) {
            subscriber.removeTimeout();
        }

        const timeout = setTimeout(() => this.#subscribers.delete(nodeId), DELETE_SUBSCRIBER_TIMEOUT_MS);
        this.#subscribers.set(nodeId, { nodeId, removeTimeout: () => clearTimeout(timeout) });
    }

    publish(frame: NotifyFrame): void {
        const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
        frame.serialize(writer);

        for (const subscriber of this.#subscribers.values()) {
            this.#socket.send(subscriber.nodeId, new BufferReader(writer.unwrapBuffer()));
        }
    }
}
