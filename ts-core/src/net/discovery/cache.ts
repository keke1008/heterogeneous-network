import { ObjectMap, UniqueKey } from "@core/object";
import { ClusterId, NodeId } from "../node";
import { DiscoveryFrameType, ReceivedDiscoveryFrame, TotalCost } from "./frame";
import { sleep } from "@core/async";
import { DISCOVERY_CACHE_EXPIRATION, MAX_DISCOVERY_CACHE_ENTRIES } from "./constants";
import { OptionalClusterId } from "../node/clusterId";

interface CacheEntry {
    gateway: NodeId;
    totalCost: TotalCost;
}

export class CacheList<T extends UniqueKey> {
    #entries = new ObjectMap<T, CacheEntry & { readonly id: symbol }>();

    add(destination: T, entry: CacheEntry) {
        const id = Symbol();
        this.#entries.set(destination, { ...entry, id });

        if (this.#entries.size > MAX_DISCOVERY_CACHE_ENTRIES) {
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
    #nodeIdCache = new CacheList<NodeId>();
    #clusterIdCache = new CacheList<ClusterId>();

    updateByReceivedFrame(frame: ReceivedDiscoveryFrame, totalCost: TotalCost) {
        const start = frame.type === DiscoveryFrameType.Request ? frame.source.intoDestination() : frame.target;

        if (!start.nodeId.isBroadcast()) {
            const nodeId = frame.target.nodeId;
            nodeId && this.#nodeIdCache.add(start.nodeId, { gateway: frame.previousHop, totalCost });
        }

        if (start.clusterId instanceof ClusterId) {
            const clusterId = frame.target.clusterId;
            clusterId && this.#clusterIdCache.add(start.clusterId, { gateway: frame.previousHop, totalCost });
        }
    }

    getCacheByNodeId(nodeId: NodeId): CacheEntry | undefined {
        return this.#nodeIdCache.get(nodeId);
    }

    getCacheByClusterId(clusterId: OptionalClusterId): CacheEntry | undefined {
        return clusterId instanceof ClusterId ? this.#clusterIdCache.get(clusterId) : undefined;
    }
}
