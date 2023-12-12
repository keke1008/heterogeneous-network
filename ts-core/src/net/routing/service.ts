import { Address } from "../link";
import { NodeId } from "../node";

export interface RoutingService {
    resolveGatewayNode(destination: NodeId): Promise<NodeId | undefined>;
    resolveMediaAddresses(destination: NodeId): Promise<Address[] | undefined>;
}
