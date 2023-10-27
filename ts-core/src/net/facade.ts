import { AddressType, FrameHandler, LinkService } from "./link";
import { Cost, NodeId, ReactiveService } from "./routing";
import { RpcService } from "./rpc";

export class NetFacade {
    #linkService: LinkService = new LinkService();
    #routingService: ReactiveService | undefined;
    #rpcService: RpcService | undefined;

    addHandler(addressType: AddressType, handler: FrameHandler) {
        this.#linkService.addHandler(addressType, handler);
        if (this.#routingService === undefined) {
            this.#routingService = new ReactiveService(this.#linkService, new NodeId(handler.address()), new Cost(0));
            this.#rpcService = new RpcService({
                linkService: this.#linkService,
                reactiveService: this.#routingService,
            });
        }
    }

    routing(): ReactiveService {
        if (this.#routingService === undefined) {
            throw new Error("Routing service is not initialized");
        }
        return this.#routingService;
    }

    rpc(): RpcService {
        if (this.#rpcService === undefined) {
            throw new Error("RPC service is not initialized");
        }
        return this.#rpcService;
    }
}
