import { EventBroker } from "@core/event";
import { Cost, NodeId } from "@core/net/node";
import { match } from "ts-pattern";

export type LocalNotification =
    | { type: "SelfUpdated"; cost: Cost }
    | { type: "NeighborUpdated"; neighborId: NodeId; neighborCost: Cost; linkCost: Cost }
    | { type: "NeighborRemoved"; nodeId: NodeId };

const toString = (event: LocalNotification): string => {
    return match(event)
        .with({ type: "SelfUpdated" }, (e) => `SelfUpdated(cost=${e.cost})`)
        .with({ type: "NeighborUpdated" }, (e) => {
            return `NeighborUpdated(neighborId=${e.neighborId}, neighborCost=${e.neighborCost}, linkCost=${e.linkCost})`;
        })
        .with({ type: "NeighborRemoved" }, (e) => `NeighborRemoved(nodeId=${e.nodeId})`)
        .exhaustive();
};

export class NotificationService {
    #onNotification = new EventBroker<LocalNotification>();

    notify(event: LocalNotification) {
        console.info(`[NotificationService] ${toString(event)}`);
        if (this.#onNotification) {
            this.#onNotification.emit(event);
        }
    }

    onNotification(handler: (event: LocalNotification) => void) {
        this.#onNotification.listen(handler);
    }
}
