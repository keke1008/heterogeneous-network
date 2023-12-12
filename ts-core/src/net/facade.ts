import { consume } from "@core/types";
import { AddressType, FrameHandler, LinkService } from "./link";
import { NeighborService } from "./neighbor";
import { LocalNodeService } from "./node";
import { NotificationService } from "./notification";
import { ObserverService } from "./observer";
import { RoutingService, ReactiveService } from "./routing";
import { RpcService } from "./rpc";
import { RoutingSocketConstructor } from "./routing/service";
import { ReactiveSocket } from "./routing/reactive/socket";

export class NetFacade {
    #linkService: LinkService;
    #localNodeService: LocalNodeService;
    #notificationService: NotificationService;
    #neighborService: NeighborService;
    #routingService: RoutingService;
    #rpcService: RpcService;
    #observerService: ObserverService;

    constructor(routingSocketConstructor: RoutingSocketConstructor = ReactiveSocket) {
        this.#linkService = new LinkService();
        this.#localNodeService = new LocalNodeService({
            linkService: this.#linkService,
        });
        this.#notificationService = new NotificationService();
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
            routingService: this.#routingService,
            routingSocketConstructor,
        });
        this.#observerService = new ObserverService({
            notificationService: this.#notificationService,
            linkService: this.#linkService,
            localNodeService: this.#localNodeService,
            neighborService: this.#neighborService,
            routingService: this.#routingService,
            routingSocketConstructor,
        });

        consume(this.#notificationService);
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

    routing(): RoutingService {
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
