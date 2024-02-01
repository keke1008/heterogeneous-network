import { Destination, DiscoveryService, NodeId } from "@core/net";
import { ConstantSerdeable, TransformSerdeable, VariantSerdeable } from "@core/serde";

export interface IGateway {
    resolve(destination: Destination, args: { discoveryService: DiscoveryService }): Promise<NodeId | undefined>;
}

export enum GatewayType {
    Unicast = 1,
    Discovery = 2,
    Discard = 3,
}

export class UnicastGateway implements IGateway {
    readonly type = GatewayType.Unicast;
    #nodeId: NodeId;

    constructor(nodeId: NodeId) {
        this.#nodeId = nodeId;
    }

    static readonly serdeable = new TransformSerdeable(
        NodeId.serdeable,
        (nodeId) => new UnicastGateway(nodeId),
        (gateway) => gateway.#nodeId,
    );

    async resolve(): Promise<NodeId> {
        return this.#nodeId;
    }
}

export class DiscoveryGateway implements IGateway {
    readonly type = GatewayType.Discovery;
    static readonly serdeable = new ConstantSerdeable(new DiscoveryGateway());

    resolve(destination: Destination, args: { discoveryService: DiscoveryService }): Promise<NodeId | undefined> {
        return args.discoveryService.resolveGatewayNode(destination);
    }
}

export class DiscardGateway implements IGateway {
    readonly type = GatewayType.Discard;
    static readonly serdeable = new ConstantSerdeable(new DiscardGateway());

    resolve(): Promise<undefined> {
        return Promise.resolve(undefined);
    }
}

export type Gateway = UnicastGateway | DiscoveryGateway | DiscardGateway;
export const Gateway = {
    serdeable: new VariantSerdeable(
        [UnicastGateway.serdeable, DiscoveryGateway.serdeable, DiscardGateway.serdeable],
        (gateway) => gateway.type,
    ),
};
