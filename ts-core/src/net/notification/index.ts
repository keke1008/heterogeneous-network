import { EventBroker } from "@core/event";
import { Cost, NodeId } from "@core/net/node";

export type LocalNotification =
    | { type: "SelfUpdated"; cost: Cost }
    | { type: "NeighborUpdated"; sourceCost: Cost; nodeId: NodeId; linkCost: Cost; nodeCost: Cost }
    | { type: "NeighborRemoved"; nodeId: NodeId };

export class NotificationService {
    #onNotification = new EventBroker<LocalNotification>();

    notify(event: LocalNotification) {
        if (this.#onNotification) {
            this.#onNotification.emit(event);
        }
    }

    onNotification(handler: (event: LocalNotification) => void) {
        this.#onNotification.listen(handler);
    }
}
