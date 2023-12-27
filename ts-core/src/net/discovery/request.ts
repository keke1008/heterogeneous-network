import { ObjectMap } from "@core/object";
import { Cost, NodeId } from "../node";
import { DiscoveryFrameType, ReceivedDiscoveryFrame } from "./frame";
import { sleep, withTimeout } from "@core/async";
import { Duration, Instant } from "@core/time";
import { deferred } from "@core/deferred";
import { Destination } from "../node";

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

    handleResponse(frame: ReceivedDiscoveryFrame) {
        const response = { gatewayId: frame.previousHop.nodeId, cost: frame.totalCost };
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
    #requests = new ObjectMap<Destination, DiscoveryRequestEntry, string>((id) => id.toUniqueString());
    #firstResponseTimeout: Duration = Duration.fromMillies(1000);
    #betterResponseTimeoutRate: number = 1;

    handleResponse(frame: ReceivedDiscoveryFrame) {
        if (frame.type !== DiscoveryFrameType.Response) {
            throw new Error("Invalid frame type");
        }
        const entry = this.#requests.get(frame.target);
        entry?.handleResponse(frame);
    }

    request(target: Destination): Promise<DiscoveryResponse | undefined> {
        const exists = this.#requests.get(target);
        if (exists !== undefined) {
            return exists.result;
        }

        const entry = new DiscoveryRequestEntry(async (entry) => {
            this.#requests.set(target, entry);

            const beginDiscovery = Instant.now();
            const firstResponse = await withTimeout({
                promise: entry.firstResponse,
                timeout: this.#firstResponseTimeout,
                onTimeout: () => undefined,
            });
            if (firstResponse === undefined) {
                this.#requests.delete(target);
                return undefined;
            }

            const elapsed = Instant.now().subtract(beginDiscovery);
            const betterResponseTimeout = elapsed.multiply(this.#betterResponseTimeoutRate);
            await sleep(betterResponseTimeout);

            this.#requests.delete(target);
            return entry.response;
        });
        this.#requests.set(target, entry);
        return entry.result;
    }

    getCurrentRequest(target: Destination): DiscoveryRequestEntry | undefined {
        return this.#requests.get(target);
    }
}
