import { Address, AddressType, FrameHandler, LinkService } from "./link";
import { Cost, NodeId, ReactiveService } from "./routing";

export class NetFacade {
    #linkService: LinkService = new LinkService();
    #routingService: ReactiveService | undefined;

    addHandler(addressType: AddressType, handler: FrameHandler) {
        this.#linkService.addHandler(addressType, handler);
        if (this.#routingService === undefined) {
            this.#routingService = new ReactiveService(this.#linkService, new NodeId(handler.address()), new Cost(0));
        }
    }

    #unwrappedRoutingService(): ReactiveService {
        if (this.#routingService === undefined) {
            throw new Error("Routing service is not initialized");
        }
        return this.#routingService;
    }

    sendHello(destination: Address, edgeCost: Cost) {
        this.#unwrappedRoutingService().requestHello(destination, edgeCost);
    }

    sendGoodbye(destination: NodeId) {
        this.#unwrappedRoutingService().requestGoodbye(destination);
    }
}
