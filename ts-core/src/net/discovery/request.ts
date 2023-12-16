import { ObjectMap } from "@core/object";
import { Cost, NodeId } from "../node";
import { DiscoveryFrame } from "./frame";
import { sleep, withTimeout } from "@core/async";
import { Duration, Instant } from "@core/time";
import { deferred } from "@core/deferred";

export interface DiscoveryResponse {
    gatewayId: NodeId;
    cost: Cost;
}

class DiscoveryRequestEntry {
    #response?: DiscoveryResponse;
    #firstResponse = deferred<DiscoveryResponse>();
    #result: Promise<DiscoveryResponse | undefined>;

    constructor(fn: (entry: DiscoveryRequestEntry) => Promise<DiscoveryResponse | undefined>) {
        this.#result = fn(this);
    }

    handleResponse(frame: DiscoveryFrame) {
        const response = { gatewayId: frame.senderId, cost: frame.totalCost };
        if (this.#response === undefined || this.#response.cost.get() > response.cost.get()) {
            this.#firstResponse.resolve(response);
            this.#response = response;
        }
    }

    get response(): DiscoveryResponse | undefined {
        return this.#response;
    }

    get firstResponse(): Promise<DiscoveryResponse> {
        return this.#firstResponse;
    }

    get result(): Promise<DiscoveryResponse | undefined> {
        return this.#result;
    }
}

export class LocalRequestStore {
    #requests = new ObjectMap<NodeId, DiscoveryRequestEntry, string>((id) => id.toString());
    #firstResponseTimeout: Duration = Duration.fromMillies(1000);
    #betterResponseTimeoutRate: number = 1;

    handleResponse(frame: DiscoveryFrame) {
        const entry = this.#requests.get(frame.sourceId);
        entry?.handleResponse(frame);
    }

    request(destinationId: NodeId): Promise<DiscoveryResponse | undefined> {
        const exists = this.#requests.get(destinationId);
        if (exists !== undefined) {
            return exists.result;
        }

        const entry = new DiscoveryRequestEntry(async (entry) => {
            this.#requests.set(destinationId, entry);

            const beginDiscovery = Instant.now();
            const firstResponse = await withTimeout({
                promise: entry.firstResponse,
                timeout: this.#firstResponseTimeout,
                onTimeout: () => undefined,
            });
            if (firstResponse === undefined) {
                this.#requests.delete(destinationId);
                return undefined;
            }

            const elapsed = Instant.now().subtract(beginDiscovery);
            const betterResponseTimeout = elapsed.multiply(this.#betterResponseTimeoutRate);
            await sleep(betterResponseTimeout);

            this.#requests.delete(destinationId);
            return entry.response;
        });
        this.#requests.set(destinationId, entry);
        return entry.result;
    }

    getCurrentRequest(destinationId: NodeId): DiscoveryRequestEntry | undefined {
        return this.#requests.get(destinationId);
    }
}
