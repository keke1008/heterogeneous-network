import { EventDispatcher } from "@core/event";
import { Cost, NodeId } from "../routing";

export type NetNotification =
    | { type: "NodeUpdated"; nodeId: NodeId; nodeCost: Cost }
    | { type: "NodeRemoved"; nodeId: NodeId }
    | { type: "LinkUpdated"; nodeId1: NodeId; nodeId2: NodeId; linkCost: Cost }
    | { type: "LinkRemoved"; nodeId1: NodeId; nodeId2: NodeId };

export class NotificationService {
    #onNotification: EventDispatcher<NetNotification> = new EventDispatcher();

    notify(event: NetNotification) {
        if (this.#onNotification) {
            this.#onNotification.emit(event);
        }
    }

    onNotification(handler: (event: NetNotification) => void) {
        this.#onNotification.setHandler(handler);
    }
}
