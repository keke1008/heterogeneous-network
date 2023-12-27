import { ObjectMap } from "@core/object";
import { ClusterId, NodeId } from "../node";
import { DiscoveryFrameType, ReceivedDiscoveryFrame, TotalCost } from "./frame";
import { Destination } from "../node";
import { Duration } from "@core/time";
import { sleep } from "@core/async";

interface CacheEntry {
    gateway: NodeId;
    totalCost: TotalCost;
}

const MAX_CACHE_SIZE = 8;
const DISCOVERY_CACHE_EXPIRATION = Duration.fromSeconds(10);

export class CacheList<T> {
    #entries: ObjectMap<T, CacheEntry & { readonly id: symbol }, string>;

    constructor(getKey: (entry: T) => string) {
        this.#entries = new ObjectMap(getKey);
    }

    add(destination: T, entry: CacheEntry) {
        const id = Symbol();
        this.#entries.set(destination, { ...entry, id });

        if (this.#entries.size > MAX_CACHE_SIZE) {
            this.#entries.delete(this.#entries.keys().next().value);
        }

        sleep(DISCOVERY_CACHE_EXPIRATION).then(() => {
            const entry = this.#entries.get(destination);
            if (entry && entry.id === id) {
                this.#entries.delete(destination);
            }
        });
    }

    get(destination: T): CacheEntry | undefined {
        return this.#entries.get(destination);
    }
}

export class DiscoveryRequestCache {
    #nodeIdCache = new CacheList<NodeId>((id) => id.toString());
    #clusterIdCache = new CacheList<ClusterId>((id) => id.toString());

    updateByReceivedFrame(frame: ReceivedDiscoveryFrame, totalCost: TotalCost) {
        const start = frame.type === DiscoveryFrameType.Request ? frame.source.intoDestination() : frame.target;

        if (start.nodeId !== undefined) {
            const nodeId = frame.target.nodeId;
            nodeId && this.#nodeIdCache.add(start.nodeId, { gateway: frame.previousHop.nodeId, totalCost });
        }

        if (start.clusterId !== undefined) {
            const clusterId = frame.target.clusterId;
            clusterId && this.#clusterIdCache.add(clusterId, { gateway: frame.previousHop.nodeId, totalCost });
        }
    }

    getCache(destination: Destination | NodeId): CacheEntry | undefined {
        const nodeId = destination instanceof NodeId ? destination : destination.nodeId;
        const entry = nodeId && this.#nodeIdCache.get(nodeId);
        if (entry) {
            return entry;
        }

        if (destination instanceof Destination) {
            const clusterId = destination.clusterId;
            return clusterId && this.#clusterIdCache.get(clusterId);
        }
    }
}
