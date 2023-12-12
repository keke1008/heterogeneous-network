import { ObjectMap } from "../../../object";
import { Cost, NodeId } from "@core/net/node";
import { RouteDiscoveryFrame } from "./frame";
import { Address } from "@core/net/link";

const sleep = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

export interface DiscoveryResponse {
    gatewayId: NodeId;
    cost: Cost;
    extra?: Address[];
}

class RequestEntry {
    #task: Promise<DiscoveryResponse | undefined>;
    #least: DiscoveryResponse | undefined = undefined;

    constructor(task: () => Promise<DiscoveryResponse | undefined>) {
        this.#task = task();
    }

    update(response: DiscoveryResponse): void {
        if (this.#least === undefined || response.cost.lessThan(this.#least.cost)) {
            this.#least = response;
        }
    }

    get(): DiscoveryResponse | undefined {
        return this.#least;
    }

    promise(): Promise<DiscoveryResponse | undefined> {
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

    get(targetId: NodeId): Promise<DiscoveryResponse | undefined> | undefined {
        return this.#requests.get(targetId)?.promise();
    }

    request(targetId: NodeId): Promise<DiscoveryResponse | undefined> {
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
            return entry.get();
        });

        this.#requests.set(targetId, entry);
        return entry.promise();
    }

    resolve(frame: RouteDiscoveryFrame, additionalCost: Cost): void {
        const entry = this.#requests.get(frame.sourceId);
        entry?.update({
            gatewayId: frame.senderId,
            cost: frame.totalCost.add(additionalCost),
            extra: Array.isArray(frame.extra) ? frame.extra : undefined,
        });
    }
}
