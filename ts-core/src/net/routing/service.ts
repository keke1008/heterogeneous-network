import { NodeId } from "../node";

export interface RoutingService {
    resolveGatewayNode(destination: NodeId): Promise<NodeId | undefined>;
}
