import { AddressType, FrameHandler, LinkService } from "./link";
import { NotificationService } from "./notification";
import { ObserverService } from "./observer";
import { Cost, NodeId } from "./node";
import { ReactiveService } from "./routing";
import { RpcService } from "./rpc";

export class NetFacade {
    #notificationService: NotificationService = new NotificationService();

    #linkService: LinkService = new LinkService();

    #routingService: ReactiveService;
    #rpcService: RpcService;
    #observerService: ObserverService;

    constructor(localId: NodeId, localCost: Cost) {
        this.#routingService = new ReactiveService({
            notificationService: this.#notificationService,
            linkService: this.#linkService,
            localId,
            localCost,
        });

        this.#rpcService = new RpcService({
            linkService: this.#linkService,
            reactiveService: this.#routingService,
        });

        this.#observerService = new ObserverService({
            notificationService: this.#notificationService,
            linkService: this.#linkService,
            reactiveService: this.#routingService,
        });
    }

    localId(): NodeId {
        return this.#routingService.localId();
    }

    localCost(): Cost {
        return this.#routingService.localCost();
    }

    addHandler(addressType: AddressType, handler: FrameHandler) {
        this.#linkService.addHandler(addressType, handler);
    }

    routing(): ReactiveService {
        return this.#routingService;
    }

    rpc(): RpcService {
        return this.#rpcService;
    }

    observer(): ObserverService {
        return this.#observerService;
    }

    dispose(): void {
        this.#observerService.dispose();
    }
}
