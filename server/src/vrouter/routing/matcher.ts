import { ClusterId, Destination, NodeId } from "@core/net";
import { ConstantSerdeable, TransformSerdeable, VariantSerdeable } from "@core/serde";

export interface IMatcher {
    isMatch(destination: Destination): boolean;
    equals(other: IMatcher): boolean;
}

export enum MatcherType {
    NodeId = 1,
    ClusterId = 2,
    Default = 3,
}

export class NodeIdMatcher implements IMatcher {
    readonly type = MatcherType.NodeId;
    #nodeId: NodeId;

    constructor(nodeId: NodeId) {
        this.#nodeId = nodeId;
    }

    static readonly serdeable = new TransformSerdeable(
        NodeId.serdeable,
        (nodeId) => new NodeIdMatcher(nodeId),
        (matcher) => matcher.#nodeId,
    );

    isMatch(destination: Destination): boolean {
        return this.#nodeId.equals(destination.nodeId);
    }

    equals(other: IMatcher): boolean {
        return other instanceof NodeIdMatcher && this.#nodeId.equals(other.#nodeId);
    }
}

export class ClusterIdMatcher implements IMatcher {
    readonly type = MatcherType.ClusterId;
    #clusterId: ClusterId;

    constructor(clusterId: ClusterId) {
        this.#clusterId = clusterId;
    }

    static readonly serdeable = new TransformSerdeable(
        ClusterId.noClusterExcludedSerdeable,
        (clusterId) => new ClusterIdMatcher(clusterId),
        (matcher) => matcher.#clusterId,
    );

    isMatch(destination: Destination): boolean {
        return this.#clusterId.equals(destination.clusterId);
    }

    equals(other: IMatcher): boolean {
        return other instanceof ClusterIdMatcher && this.#clusterId.equals(other.#clusterId);
    }
}

export class DefaultMatcher implements IMatcher {
    readonly type = MatcherType.Default;

    static readonly serdeable = new ConstantSerdeable(new DefaultMatcher());

    isMatch(): boolean {
        return true;
    }

    equals(other: IMatcher): boolean {
        return other instanceof DefaultMatcher;
    }
}

export type Matcher = NodeIdMatcher | ClusterIdMatcher | DefaultMatcher;
export const Matcher = {
    serdeable: new VariantSerdeable(
        [NodeIdMatcher.serdeable, ClusterIdMatcher.serdeable, DefaultMatcher.serdeable],
        (matcher) => matcher.type,
    ),
};
