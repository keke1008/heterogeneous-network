import { SingleListenerEventBroker } from "@core/event";
import { BufferWriter } from "../buffer";
import { Destination, NetworkState, NetworkTopologyUpdate } from "../node";
import { RoutingSocket } from "../routing";
import { NOTIFY_NETWORK_SUBSCRIPTION_INTERVAL_MS } from "./constants";
import { NetworkNotificationFrame, NetworkSubscriptionFrame, ObserverFrame } from "./frame";
import { NeighborService } from "../neighbor";
import { NetworkUpdate } from "./frame/network";

interface Cancel {
    (): void;
}

class NetworkSubscriptionSender {
    #cancel?: Cancel;

    constructor(args: { socket: RoutingSocket; neighborService: NeighborService }) {
        const { socket, neighborService } = args;

        const sendSubscription = async (destination = Destination.broadcast()) => {
            const frame = new NetworkSubscriptionFrame();
            const buffer = BufferWriter.serialize(ObserverFrame.serdeable.serializer(frame)).expect(
                "Failed to serialize frame",
            );
            await socket.send(destination, buffer);
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

        const topologyUpdates = receivedUpdates.filter(NetworkUpdate.isTopologyUpdate);
        const actualTopologyUpdates = this.#networkState.applyUpdates(topologyUpdates);

        const frameReceivedUpdates = receivedUpdates.filter(NetworkUpdate.isFrameReceived);

        this.#onNetworkUpdated.emit([...actualTopologyUpdates, ...frameReceivedUpdates]);
    }

    onNetworkUpdated(listener: (updates: NetworkUpdate[]) => void): Cancel {
        return this.#onNetworkUpdated.listen(listener);
    }

    dumpNetworkStateAsUpdates(): NetworkTopologyUpdate[] {
        return this.#networkState.dumpAsUpdates();
    }

    close() {
        this.#subscriptionSender.close();
    }
}
