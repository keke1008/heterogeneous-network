import { Destination, DiscoveryService } from "../discovery";
import { NodeId } from "../node";

export interface RoutingService {
    resolveGatewayNode(destination: Destination): Promise<NodeId | undefined>;
}

export class ReactiveRoutingService implements RoutingService {
    #discoveryService: DiscoveryService;

    constructor(args: { discoveryService: DiscoveryService }) {
        this.#discoveryService = args.discoveryService;
    }

    resolveGatewayNode(destination: Destination): Promise<NodeId | undefined> {
        return this.#discoveryService.resolveGatewayNode(destination);
    }
}
