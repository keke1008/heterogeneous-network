import { ObjectMap } from "@core/object";
import { ClusterId, NodeId } from "../node";
import { DiscoveryFrame } from "./frame";
import { Destination } from "../node/destination";

interface CacheEntry {
    gateway: NodeId;
}

const MAX_CACHE_SIZE = 8;

export class CacheList<T> {
    #entries: ObjectMap<T, CacheEntry, string>;

    constructor(getKey: (entry: T) => string) {
        this.#entries = new ObjectMap(getKey);
    }

    add(destination: T, entry: CacheEntry) {
        this.#entries.set(destination, entry);

        if (this.#entries.size > MAX_CACHE_SIZE) {
            this.#entries.delete(this.#entries.keys().next().value);
        }
    }

    get(destination: T): CacheEntry | undefined {
        return this.#entries.get(destination);
    }

    remove(destination: T) {
        this.#entries.delete(destination);
    }
}

export class DiscoveryRequestCache {
    #nodeIdCache = new CacheList<NodeId>((id) => id.toString());
    #clusterIdCache = new CacheList<ClusterId>((id) => id.toString());

    update(frame: DiscoveryFrame) {
        const nodeId = frame.target.nodeId;
        nodeId && this.#nodeIdCache.add(nodeId, { gateway: frame.sender.nodeId });

        const clusterId = frame.target.clusterId;
        clusterId && this.#clusterIdCache.add(clusterId, { gateway: frame.sender.nodeId });
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
