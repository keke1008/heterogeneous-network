import { ObjectMap } from "@core/object";
import { Cost, NodeId } from "../node";
import { DiscoveryResponseFrame, Extra, DiscoveryExtraType } from "./frame";
import { sleepMs } from "@core/async";

export interface DiscoveryResponse {
    gatewayId: NodeId;
    cost: Cost;
    extra: Extra;
}

class DiscoveryRequestEntry {
    #extraType: DiscoveryExtraType;
    #response?: DiscoveryResponse;
    #result: Promise<DiscoveryResponse | undefined>;

    constructor(
        extraType: DiscoveryExtraType,
        fn: (entry: DiscoveryRequestEntry) => Promise<DiscoveryResponse | undefined>,
    ) {
        this.#extraType = extraType;
        this.#result = fn(this);
    }

    handleResponse(frame: DiscoveryResponseFrame) {
        const response = {
            gatewayId: frame.commonFields.senderId,
            cost: frame.commonFields.totalCost,
            extra: frame.extra,
        };
        if (this.#response === undefined || this.#response.cost.get() > response.cost.get()) {
            this.#response = response;
        }
    }

    get response(): DiscoveryResponse | undefined {
        return this.#response;
    }

    get result(): Promise<DiscoveryResponse | undefined> {
        return this.#result;
    }

    get extraType(): DiscoveryExtraType {
        return this.#extraType;
    }
}

export class LocalRequestStore {
    #requests = new ObjectMap<NodeId, DiscoveryRequestEntry, string>((id) => id.toString());
    #firstResponseTimeoutMs: number = 3000;
    #betterResponseTimeoutMs: number = 1000;

    handleResponse(frame: DiscoveryResponseFrame) {
        const entry = this.#requests.get(frame.commonFields.destinationId);
        entry?.handleResponse(frame);
    }

    request(extraType: DiscoveryExtraType, destinationId: NodeId): Promise<DiscoveryResponse | undefined> {
        const exists = this.#requests.get(destinationId);
        if (exists !== undefined) {
            return exists.result;
        }

        const entry = new DiscoveryRequestEntry(extraType, async (entry) => {
            this.#requests.set(destinationId, entry);

            await sleepMs(this.#firstResponseTimeoutMs);
            const firstResponse = entry.response;
            if (firstResponse === undefined) {
                this.#requests.delete(destinationId);
                return undefined;
            }

            await sleepMs(this.#betterResponseTimeoutMs);
            this.#requests.delete(destinationId);
            return entry.response;
        });
        return entry.result;
    }

    getCurrentRequest(destinationId: NodeId): DiscoveryRequestEntry | undefined {
        return this.#requests.get(destinationId);
    }
}
