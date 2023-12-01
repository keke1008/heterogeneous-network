import { Cost, NetFacade, NetNotification, NodeId } from "@core/net";
import { LinkState } from "./state";
import { StateUpdate } from "./update";
import { Fetcher } from "./fetcher";
import { LocalNotificationSubscriber } from "@core/net/observer/subscribe";
import { CancelListening, EventBroker } from "@core/event";

class LinkStateSubscriber implements LocalNotificationSubscriber {
    #state: LinkState;
    #onStateUpdate = new EventBroker<StateUpdate>();

    constructor(localId: NodeId, localCost?: Cost) {
        const [state, update] = LinkState.create(localId, localCost);
        this.#state = state;
        this.#onStateUpdate.emit(update);
    }

    onStateUpdate(onStateUpdate: (update: StateUpdate) => void): CancelListening {
        return this.#onStateUpdate.listen(onStateUpdate);
    }

    #applyUpdate(notification: NetNotification): StateUpdate {
        switch (notification.type) {
            case "NodeUpdated":
                return this.#state.createOrUpdateNode(notification.nodeId, notification.nodeCost);
            case "NodeRemoved":
                return this.#state.removeNode(notification.nodeId);
            case "LinkUpdated":
                return this.#state.createOrUpdateLink(
                    notification.nodeId1,
                    notification.nodeId2,
                    notification.linkCost,
                );
            case "LinkRemoved":
                return this.#state.removeLink(notification.nodeId1, notification.nodeId2);
        }
    }

    onNotification(notification: NetNotification): void {
        const update = this.#applyUpdate(notification);
        this.#onStateUpdate.emit(update);
    }

    getLinksNotYetFetchedNodes(): NodeId[] {
        return this.#state.getLinksNotYetFetchedNodes();
    }

    getNodesNotYetFetchedCosts(): NodeId[] {
        return this.#state.getNodesNotYetFetchedCosts();
    }

    syncState(): StateUpdate {
        return this.#state.syncState();
    }
}

export class LinkStateService {
    #subscriber: LinkStateSubscriber;
    #fetcher: Fetcher;

    constructor(net: NetFacade) {
        this.#subscriber = new LinkStateSubscriber(net.localId(), net.localCost());
        net.observer().requestSubscribe(this.#subscriber);

        this.#fetcher = new Fetcher(net.rpc());
        this.#fetcher.onReceive((notification) => this.#subscriber.onNotification(notification));
    }

    onStateUpdate(onStateUpdate: (update: StateUpdate) => void): CancelListening {
        return this.#subscriber.onStateUpdate((update) => {
            onStateUpdate(update);
            this.#fetcher.requestFetchLinks(this.#subscriber.getLinksNotYetFetchedNodes());
            this.#fetcher.requestFetchCost(this.#subscriber.getNodesNotYetFetchedCosts());
        });
    }

    syncState(): StateUpdate {
        return this.#subscriber.syncState();
    }
}
