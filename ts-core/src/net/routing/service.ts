import { DiscoveryService } from "../discovery";
import { Address } from "../link";
import { NodeId } from "../node";

export interface RoutingService {
    resolveGatewayNode(destination: NodeId): Promise<NodeId | undefined>;
    resolveMediaAddresses(destination: NodeId): Promise<Address[] | undefined>;
}

export class ReactiveRoutingService implements RoutingService {
    #discoveryService: DiscoveryService;

    constructor(args: { discoveryService: DiscoveryService }) {
        this.#discoveryService = args.discoveryService;
    }

    resolveGatewayNode(destination: NodeId): Promise<NodeId | undefined> {
        return this.#discoveryService.resolveGatewayNode(destination);
    }

    resolveMediaAddresses(destination: NodeId): Promise<Address[] | undefined> {
        return this.#discoveryService.resolveMediaAddresses(destination);
    }
}
