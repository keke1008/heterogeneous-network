import { AddressType, FrameHandler, LinkService } from "./link";
import { NeighborService } from "./neighbor";
import { LocalNodeService } from "./node";
import { NotificationService } from "./notification";
import { ObserverService } from "./observer";
import { ReactiveService } from "./routing";
import { RpcService } from "./rpc";

export class NetFacade {
    #notificationService: NotificationService = new NotificationService();
    #linkService: LinkService = new LinkService();
    #localNodeService: LocalNodeService;
    #neighborService: NeighborService;
    #routingService: ReactiveService;
    #rpcService: RpcService;
    #observerService: ObserverService;

    constructor() {
        this.#localNodeService = new LocalNodeService({
            linkService: this.#linkService,
        });

        this.#neighborService = new NeighborService({
            notificationService: this.#notificationService,
            linkService: this.#linkService,
            localNodeService: this.#localNodeService,
        });

        this.#routingService = new ReactiveService({
            linkService: this.#linkService,
            localNodeService: this.#localNodeService,
            neighborService: this.#neighborService,
        });

        this.#rpcService = new RpcService({
            linkService: this.#linkService,
            localNodeService: this.#localNodeService,
            neighborService: this.#neighborService,
            reactiveService: this.#routingService,
        });

        this.#observerService = new ObserverService({
            notificationService: this.#notificationService,
            linkService: this.#linkService,
            localNodeService: this.#localNodeService,
            neighborService: this.#neighborService,
            reactiveService: this.#routingService,
        });
    }

    addHandler(addressType: AddressType, handler: FrameHandler) {
        this.#linkService.addHandler(addressType, handler);
    }

    localNode(): LocalNodeService {
        return this.#localNodeService;
    }

    neighbor(): NeighborService {
        return this.#neighborService;
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
        this.#observerService.close();
    }
}
