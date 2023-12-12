import { Address } from "../link";
import { NodeId } from "../node";

export interface ResolveWithMediaAddressesResult {
    gatewayId: NodeId;
    extra?: Address[];
}

export interface RoutingService {
    resolveGatewayNode(destination: NodeId): Promise<NodeId | undefined>;
    resolveGatewayNodeWithMediaAddresses(destination: NodeId): Promise<ResolveWithMediaAddressesResult | undefined>;
}
