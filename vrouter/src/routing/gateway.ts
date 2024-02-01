import { AbortResolve, NodeId, ResolveByDiscovery, ResolveResult } from "@core/net";
import { ConstantSerdeable, TransformSerdeable, VariantSerdeable } from "@core/serde";

interface IGateway {
    resolve(): ResolveResult;
}

export enum GatewayType {
    Unicast = 1,
    Discovery = 2,
    Discard = 3,
}

export class UnicastGateway implements IGateway {
    readonly type = GatewayType.Unicast;
    nodeId: NodeId;

    constructor(nodeId: NodeId) {
        this.nodeId = nodeId;
    }

    static readonly serdeable = new TransformSerdeable(
        NodeId.serdeable,
        (nodeId) => new UnicastGateway(nodeId),
        (gateway) => gateway.nodeId,
    );

    resolve(): ResolveResult {
        return this.nodeId;
    }

    display(): string {
        return this.nodeId.toHumanReadableString();
    }

    clone(): UnicastGateway {
        return new UnicastGateway(this.nodeId.clone());
    }
}

export class DiscoveryGateway implements IGateway {
    readonly type = GatewayType.Discovery;
    static readonly serdeable = new ConstantSerdeable(new DiscoveryGateway());

    resolve(): ResolveResult {
        return new ResolveByDiscovery();
    }

    display(): string {
        return "Discovery";
    }

    clone(): DiscoveryGateway {
        return new DiscoveryGateway();
    }
}

export class DiscardGateway implements IGateway {
    readonly type = GatewayType.Discard;
    static readonly serdeable = new ConstantSerdeable(new DiscardGateway());

    resolve(): ResolveResult {
        return new AbortResolve();
    }

    display(): string {
        return "Discard";
    }

    clone(): DiscardGateway {
        return new DiscardGateway();
    }
}

export type Gateway = UnicastGateway | DiscoveryGateway | DiscardGateway;
export const Gateway = {
    serdeable: new VariantSerdeable(
        [UnicastGateway.serdeable, DiscoveryGateway.serdeable, DiscardGateway.serdeable],
        (gateway) => gateway.type,
    ),
};
