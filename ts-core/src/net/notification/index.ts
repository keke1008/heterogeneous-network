import { EventBroker } from "@core/event";
import { ClusterId, Cost, NoCluster, NodeId, Source } from "@core/net/node";
import { match } from "ts-pattern";

export type LocalNotification =
    | { type: "SelfUpdated"; clusterId: ClusterId | NoCluster; cost: Cost }
    | { type: "NeighborUpdated"; neighbor: Source; neighborCost: Cost; linkCost: Cost }
    | { type: "NeighborRemoved"; nodeId: NodeId }
    | { type: "FrameReceived" };

const toString = (event: LocalNotification): string => {
    return match(event)
        .with({ type: "SelfUpdated" }, (e) => `SelfUpdated(cost=${e.cost})`)
        .with({ type: "NeighborUpdated" }, (e) => {
            return `NeighborUpdated(neighborId=${e.neighbor}, neighborCost=${e.neighborCost}, linkCost=${e.linkCost})`;
        })
        .with({ type: "NeighborRemoved" }, (e) => `NeighborRemoved(nodeId=${e.nodeId})`)
        .with({ type: "FrameReceived" }, () => `FrameReceived`)
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
