import { SingleListenerEventBroker } from "@core/event";
import { BufferReader, BufferWriter } from "../buffer";
import { NetworkState, NetworkUpdate } from "../node";
import { RoutingSocket } from "../routing";
import { NOTIFY_NETWORK_SUBSCRIPTION_INTERVAL_MS } from "./constants";
import { NetworkSubscriptionFrame } from "./frame";
import { NetworkNotificationFrame } from "./frame/network";

interface Cancel {
    (): void;
}

class NetworkSubscriptionSender {
    #cancel: Cancel;

    constructor(socket: RoutingSocket) {
        const sendSubscription = () => {
            const frame = new NetworkSubscriptionFrame();
            const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
            frame.serialize(writer);
            socket.sendBroadcast(new BufferReader(writer.unwrapBuffer()));
        };

        sendSubscription();
        const timeout = setInterval(sendSubscription, NOTIFY_NETWORK_SUBSCRIPTION_INTERVAL_MS);
        this.#cancel = () => clearInterval(timeout);
    }

    close() {
        this.#cancel();
    }
}

export class ClientService {
    #networkState = new NetworkState();
    #subscriptionSender: NetworkSubscriptionSender;
    #onNetworkUpdated: SingleListenerEventBroker<NetworkUpdate[]> = new SingleListenerEventBroker();

    constructor(socket: RoutingSocket) {
        this.#subscriptionSender = new NetworkSubscriptionSender(socket);
    }

    dispatchReceivedFrame(frame: NetworkNotificationFrame) {
        const receivedUpdates = frame.entries.map((entry) => entry.toNetworkUpdate());
        const actualUpdates = this.#networkState.applyUpdates(receivedUpdates);
        this.#onNetworkUpdated.emit(actualUpdates);
    }

    onNetworkUpdated(listener: (updates: NetworkUpdate[]) => void): Cancel {
        return this.#onNetworkUpdated.listen(listener);
    }

    close() {
        this.#subscriptionSender.close();
    }
}
