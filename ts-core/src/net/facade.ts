import { consume } from "@core/types";
import { AddressType, FrameHandler, LinkService } from "./link";
import { NeighborService } from "./neighbor";
import { LocalNodeService } from "./node";
import { NotificationService } from "./notification";
import { ObserverService } from "./observer";
import { RoutingService, ReactiveService } from "./routing";
import { RpcService } from "./rpc";

export class NetFacade {
    #linkService: LinkService = new LinkService();
    #localNodeService: LocalNodeService;
    #notificationService: NotificationService = new NotificationService();
    #neighborService: NeighborService;
    #routingService: RoutingService;
    #rpcService: RpcService;
    #observerService: ObserverService;

    constructor(args: {
        linkService: LinkService;
        notificationService: NotificationService;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        routingService: RoutingService;
        rpcService: RpcService;
        observerService: ObserverService;
    }) {
        this.#linkService = args.linkService;
        this.#localNodeService = args.localNodeService;
        this.#notificationService = args.notificationService;
        this.#neighborService = args.neighborService;
        this.#routingService = args.routingService;
        this.#rpcService = args.rpcService;
        this.#observerService = args.observerService;

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

export class NetFacadeBuilder {
    #linkService?: LinkService;
    #localNodeService?: LocalNodeService;
    #notificationService?: NotificationService;
    #neighborService?: NeighborService;
    #routingService?: RoutingService;
    #rpcService?: RpcService;
    #observerService?: ObserverService;

    #getOrDefaultLinkService(): LinkService {
        this.#linkService ??= new LinkService();
        return this.#linkService;
    }

    #getOrDefaultLocalNodeService(): LocalNodeService {
        this.#localNodeService ??= new LocalNodeService({
            linkService: this.#getOrDefaultLinkService(),
        });
        return this.#localNodeService;
    }

    #getOrDefaultNotificationService(): NotificationService {
        this.#notificationService ??= new NotificationService();
        return this.#notificationService;
    }

    #getOrDefaultNeighborService(): NeighborService {
        this.#neighborService ??= new NeighborService({
            notificationService: this.#getOrDefaultNotificationService(),
            linkService: this.#getOrDefaultLinkService(),
            localNodeService: this.#getOrDefaultLocalNodeService(),
        });
        return this.#neighborService;
    }

    #getOrDefaultRoutingService(): RoutingService {
        this.#routingService ??= new ReactiveService({
            linkService: this.#getOrDefaultLinkService(),
            localNodeService: this.#getOrDefaultLocalNodeService(),
            neighborService: this.#getOrDefaultNeighborService(),
        });
        return this.#routingService;
    }

    #getOrDefaultRpcService(): RpcService {
        this.#rpcService ??= new RpcService({
            linkService: this.#getOrDefaultLinkService(),
            localNodeService: this.#getOrDefaultLocalNodeService(),
            neighborService: this.#getOrDefaultNeighborService(),
            routingService: this.#getOrDefaultRoutingService(),
        });
        return this.#rpcService;
    }

    #getOrDefaultObserverService(): ObserverService {
        this.#observerService ??= new ObserverService({
            notificationService: this.#getOrDefaultNotificationService(),
            linkService: this.#getOrDefaultLinkService(),
            localNodeService: this.#getOrDefaultLocalNodeService(),
            neighborService: this.#getOrDefaultNeighborService(),
            routingService: this.#getOrDefaultRoutingService(),
        });
        return this.#observerService;
    }

    withDefaultLinkService(): LinkService {
        if (this.#linkService !== undefined) {
            throw new Error("LinkService is already set");
        }
        return this.#getOrDefaultLinkService();
    }

    withDefaultLocalNodeService(): LocalNodeService {
        if (this.#localNodeService !== undefined) {
            throw new Error("LocalNodeService is already set");
        }
        return this.#getOrDefaultLocalNodeService();
    }

    withDefaultNotificationService(): NotificationService {
        if (this.#notificationService !== undefined) {
            throw new Error("NotificationService is already set");
        }
        return this.#getOrDefaultNotificationService();
    }

    withDefaultNeighborService(): NeighborService {
        if (this.#neighborService !== undefined) {
            throw new Error("NeighborService is already set");
        }
        return this.#getOrDefaultNeighborService();
    }

    withRoutingService(routingService: RoutingService): RoutingService {
        if (this.#routingService !== undefined) {
            throw new Error("RoutingService is already set");
        }
        this.#routingService = routingService;
        return routingService;
    }

    withDefaultRoutingService(): RoutingService {
        if (this.#routingService !== undefined) {
            throw new Error("RoutingService is already set");
        }
        return this.#getOrDefaultRoutingService();
    }

    buildWithDefaults(): NetFacade {
        return new NetFacade({
            linkService: this.#getOrDefaultLinkService(),
            localNodeService: this.#getOrDefaultLocalNodeService(),
            notificationService: this.#getOrDefaultNotificationService(),
            neighborService: this.#getOrDefaultNeighborService(),
            routingService: this.#getOrDefaultRoutingService(),
            rpcService: this.#getOrDefaultRpcService(),
            observerService: this.#getOrDefaultObserverService(),
        });
    }
}
