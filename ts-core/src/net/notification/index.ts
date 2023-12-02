import { EventBroker } from "@core/event";
import { Cost, NodeId } from "@core/net/node";

export type LocalNotification =
    | { type: "NodeUpdated"; nodeId: NodeId; nodeCost: Cost }
    | { type: "NodeRemoved"; nodeId: NodeId }
    | { type: "LinkUpdated"; nodeId1: NodeId; nodeId2: NodeId; linkCost: Cost }
    | { type: "LinkRemoved"; nodeId1: NodeId; nodeId2: NodeId };

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
