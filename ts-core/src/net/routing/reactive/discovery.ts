import { ObjectMap } from "../../../object";
import { Cost, NodeId } from "../node";

const sleep = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

class RequestEntry {
    #task: Promise<NodeId | undefined>;
    #least: { gatewayId: NodeId; cost: Cost } | undefined = undefined;

    constructor(task: () => Promise<NodeId | undefined>) {
        this.#task = task();
    }

    update(gatewayId: NodeId, cost: Cost) {
        if (this.#least === undefined || cost.lessThan(this.#least.cost)) {
            this.#least = { gatewayId, cost };
        }
    }

    get(): { gatewayId: NodeId; cost: Cost } | undefined {
        return this.#least;
    }

    promise(): Promise<NodeId | undefined> {
        return this.#task;
    }
}

export class RouteDiscoveryRequests {
    #requests: ObjectMap<NodeId, RequestEntry, string> = new ObjectMap((n) => n.toString());
    #firstResponseTimeoutMs: number;
    #betterResponseTimeoutMs: number;

    constructor(option?: { firstResponseTimeoutMs?: number; betterResponseTimeoutMs?: number }) {
        this.#firstResponseTimeoutMs = option?.firstResponseTimeoutMs ?? 3000;
        this.#betterResponseTimeoutMs = option?.betterResponseTimeoutMs ?? 1000;
    }

    get(targetId: NodeId): Promise<NodeId | undefined> | undefined {
        return this.#requests.get(targetId)?.promise();
    }

    request(targetId: NodeId): Promise<NodeId | undefined> {
        const prev = this.#requests.get(targetId);
        if (prev !== undefined) {
            return prev.promise();
        }

        const entry: RequestEntry = new RequestEntry(async () => {
            await sleep(this.#firstResponseTimeoutMs);
            const least = entry.get();
            if (least === undefined) {
                return;
            }

            await sleep(this.#betterResponseTimeoutMs);
            this.#requests.delete(targetId);
            return entry.get()?.gatewayId;
        });

        this.#requests.set(targetId, entry);
        return entry.promise();
    }

    resolve(targetId: NodeId, gatewayId: NodeId, cost: Cost): void {
        const entry = this.#requests.get(targetId);
        if (entry !== undefined) {
            entry.update(gatewayId, cost);
        }
    }
}
