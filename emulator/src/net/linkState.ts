import { NetFacade, NetworkUpdate } from "@core/net";
import { CancelListening, EventBroker } from "@core/event";

export class LinkStateService {
    #onNetworkUpdate = new EventBroker<NetworkUpdate[]>();

    constructor(net: NetFacade) {
        net.observer().launchClientService((update) => this.#onNetworkUpdate.emit(update));
    }

    onNetworkUpdate(onStateUpdate: (update: NetworkUpdate[]) => void): CancelListening {
        return this.#onNetworkUpdate.listen(onStateUpdate);
    }

    dumpNetworkStateAsUpdate(net: NetFacade): NetworkUpdate[] {
        return net.observer().dumpNetworkStateAsUpdates();
    }
}
