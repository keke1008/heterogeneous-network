import { SingleListenerEventBroker } from "@core/event";
import { BufferReader, BufferWriter } from "../buffer";
import { Destination, NetworkState, NetworkUpdate } from "../node";
import { RoutingSocket } from "../routing";
import { NOTIFY_NETWORK_SUBSCRIPTION_INTERVAL_MS } from "./constants";
import { NetworkNotificationFrame, NetworkSubscriptionFrame } from "./frame";
import { NeighborService } from "../neighbor";

interface Cancel {
    (): void;
}

class NetworkSubscriptionSender {
    #cancel?: Cancel;

    constructor(args: { socket: RoutingSocket; neighborService: NeighborService }) {
        const { socket, neighborService } = args;

        const sendSubscription = async (destination = Destination.broadcast()) => {
            const frame = new NetworkSubscriptionFrame();
            const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
            frame.serialize(writer);
            await socket.send(destination, new BufferReader(writer.unwrapBuffer()));
        };

        sendSubscription().then(() => {
            const timeout = setInterval(sendSubscription, NOTIFY_NETWORK_SUBSCRIPTION_INTERVAL_MS);
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

export class ClientService {
    #networkState = new NetworkState();
    #subscriptionSender: NetworkSubscriptionSender;
    #onNetworkUpdated: SingleListenerEventBroker<NetworkUpdate[]> = new SingleListenerEventBroker();

    constructor(args: { neighborService: NeighborService; socket: RoutingSocket }) {
        this.#subscriptionSender = new NetworkSubscriptionSender(args);
    }

    dispatchReceivedFrame(frame: NetworkNotificationFrame) {
        const receivedUpdates = frame.entries.map((entry) => entry.toNetworkUpdate());
        const actualUpdates = this.#networkState.applyUpdates(receivedUpdates);
        this.#onNetworkUpdated.emit(actualUpdates);
    }

    onNetworkUpdated(listener: (updates: NetworkUpdate[]) => void): Cancel {
        return this.#onNetworkUpdated.listen(listener);
    }

    dumpNetworkStateAsUpdates(): NetworkUpdate[] {
        return this.#networkState.dumpAsUpdates();
    }

    close() {
        this.#subscriptionSender.close();
    }
}
