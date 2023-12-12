import {
    RoutingService,
    DiscoveryService,
    NodeId,
    UdpAddress,
    NeighborService,
    AddressClass,
    Cost,
    NeighborNode,
    LocalNodeService,
} from "@core/net";
import { DiscoveryRequestFrame } from "@core/net/discovery/frame";

interface ResolvedRoute {
    gatewayNode: NeighborNode;
    gatewayAddress: UdpAddress;
    destinationAddress: UdpAddress;
    totalCost: Cost;
}

class VRouterRoutingService implements RoutingService {
    #localNodeService: LocalNodeService;
    #neighborService: NeighborService;
    #discoveryService: DiscoveryService;

    constructor(args: {
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        discoveryService: DiscoveryService;
    }) {
        this.#localNodeService = args.localNodeService;
        this.#neighborService = args.neighborService;
        this.#discoveryService = args.discoveryService;
        this.#discoveryService.onReceiveRepeatingRequest((request) => this.#handleRepeatingRequest(request));
    }

    async #handleRepeatingRequest(request: DiscoveryRequestFrame): Promise<void> {
        const senderNode = this.#neighborService.getNeighbor(request.commonFields.senderId);
        if (senderNode === undefined) {
            return;
        }

        const route = await this.#resolveGatewayNode(request.commonFields.destinationId);
        if (route === undefined) {
            return;
        }

        // -- add routing table entry

        const localCost = await this.#localNodeService.getCost();
        const initialCost = request.commonFields.totalCost
            .add(senderNode.edgeCost)
            .add(localCost)
            .add(route.gatewayNode.edgeCost)
            .add(route.totalCost);
        this.#discoveryService.replyToRequestFrame(request, { initialCost });
    }

    async #resolveGatewayNode(destination: NodeId): Promise<ResolvedRoute | undefined> {
        const result = await this.#discoveryService.resolveMediaAddresses(destination);
        if (result === undefined) {
            return;
        }

        const filterUdpAddress = (address: AddressClass): address is UdpAddress => address instanceof UdpAddress;

        const destinationUdpAddress = result.extra.map((a) => a.address).find(filterUdpAddress);
        if (destinationUdpAddress === undefined) {
            return;
        }

        const gatewayNode = this.#neighborService.getNeighbor(result.gatewayId);
        if (gatewayNode === undefined) {
            return;
        }

        const gatewayUdpAddress = gatewayNode.addresses.map((a) => a.address).find(filterUdpAddress);
        if (gatewayUdpAddress === undefined) {
            return;
        }

        return {
            gatewayNode,
            gatewayAddress: gatewayUdpAddress,
            destinationAddress: destinationUdpAddress,
            totalCost: result.cost,
        };
    }

    async resolveGatewayNode(destination: NodeId): Promise<NodeId | undefined> {
        const route = await this.#resolveGatewayNode(destination);
        return route?.gatewayNode.id;
    }
}
