import { NetFacade, NetworkTopologyUpdate } from "@core/net";
import { CancelListening, EventBroker } from "@core/event";
import { NetworkUpdate } from "@core/net/observer/frame";

export class LinkStateService {
    #onNetworkUpdate = new EventBroker<NetworkUpdate[]>();

    constructor(net: NetFacade) {
        net.observer().launchClientService((update) => this.#onNetworkUpdate.emit(update));
    }

    onNetworkUpdate(onStateUpdate: (update: NetworkUpdate[]) => void): CancelListening {
        return this.#onNetworkUpdate.listen(onStateUpdate);
    }

    dumpNetworkStateAsUpdate(net: NetFacade): NetworkTopologyUpdate[] {
        return net.observer().dumpNetworkStateAsUpdates();
    }
}
