import { consume } from "@core/types";
import { AddressType, FrameHandler, LinkService } from "./link";
import { NeighborService } from "./neighbor";
import { NotificationService } from "./notification";
import { ObserverService } from "./observer";
import { RoutingService, ReactiveRoutingService } from "./routing";
import { RpcService } from "./rpc";
import { DiscoveryService } from "./discovery";
import { LocalNodeService } from "./local";
import { TunnelService } from "./tunnel";
import { TrustedService } from "./trusted";
import { StreamService } from "./stream";

export class NetFacade {
    #linkService: LinkService;
    #localNodeService: LocalNodeService;
    #notificationService: NotificationService;
    #neighborService: NeighborService;
    #discoveryService: DiscoveryService;
    #routingService: RoutingService;
    #rpcService: RpcService;
    #observerService: ObserverService;
    #tunnelService: TunnelService;
    #trustedService: TrustedService;
    #streamService: StreamService;

    constructor(args: {
        linkService: LinkService;
        notificationService: NotificationService;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        discoveryService: DiscoveryService;
        routingService: RoutingService;
        rpcService: RpcService;
        observerService: ObserverService;
        tunnelService: TunnelService;
        trustedService: TrustedService;
        streamService: StreamService;
    }) {
        this.#linkService = args.linkService;
        this.#localNodeService = args.localNodeService;
        this.#notificationService = args.notificationService;
        this.#neighborService = args.neighborService;
        this.#discoveryService = args.discoveryService;
        this.#routingService = args.routingService;
        this.#rpcService = args.rpcService;
        this.#observerService = args.observerService;
        this.#tunnelService = args.tunnelService;
        this.#trustedService = args.trustedService;
        this.#streamService = args.streamService;

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

    discovery(): DiscoveryService {
        return this.#discoveryService;
    }

    rpc(): RpcService {
        return this.#rpcService;
    }

    observer(): ObserverService {
        return this.#observerService;
    }

    tunnel(): TunnelService {
        return this.#tunnelService;
    }

    trusted(): TrustedService {
        return this.#trustedService;
    }

    stream(): StreamService {
        return this.#streamService;
    }

    dispose(): void {
        this.#observerService.close();
    }
}

export class NetFacadeBuilder {
    #linkService?: LinkService;
    #notificationService?: NotificationService;
    #localNodeService?: LocalNodeService;
    #neighborService?: NeighborService;
    #discoveryService?: DiscoveryService;
    #routingService?: RoutingService;
    #rpcService?: RpcService;
    #observerService?: ObserverService;
    #tunnelService?: TunnelService;
    #trustedService?: TrustedService;
    #streamService?: StreamService;

    #getOrDefaultLinkService(): LinkService {
        this.#linkService ??= new LinkService();
        return this.#linkService;
    }

    #getOrDefaultNotificationService(): NotificationService {
        this.#notificationService ??= new NotificationService();
        return this.#notificationService;
    }

    #getOrDefaultLocalNodeService(): LocalNodeService {
        this.#localNodeService ??= new LocalNodeService({
            linkService: this.#getOrDefaultLinkService(),
            notificationService: this.#getOrDefaultNotificationService(),
        });
        return this.#localNodeService;
    }

    #getOrDefaultNeighborService(): NeighborService {
        this.#neighborService ??= new NeighborService({
            notificationService: this.#getOrDefaultNotificationService(),
            linkService: this.#getOrDefaultLinkService(),
            localNodeService: this.#getOrDefaultLocalNodeService(),
        });
        return this.#neighborService;
    }

    #getOrDefaultDiscoveryService(): DiscoveryService {
        this.#discoveryService ??= new DiscoveryService({
            linkService: this.#getOrDefaultLinkService(),
            localNodeService: this.#getOrDefaultLocalNodeService(),
            neighborService: this.#getOrDefaultNeighborService(),
        });
        return this.#discoveryService;
    }

    #getOrDefaultRoutingService(): RoutingService {
        this.#routingService ??= new ReactiveRoutingService({
            discoveryService: this.#getOrDefaultDiscoveryService(),
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

    #getOrDefaultTunnelService(): TunnelService {
        this.#tunnelService ??= new TunnelService({
            linkService: this.#getOrDefaultLinkService(),
            notificationService: this.#getOrDefaultNotificationService(),
            localNodeService: this.#getOrDefaultLocalNodeService(),
            neighborService: this.#getOrDefaultNeighborService(),
            routingService: this.#getOrDefaultRoutingService(),
        });
        return this.#tunnelService;
    }

    #getOrDefaultTrustedService(): TrustedService {
        this.#trustedService ??= new TrustedService({
            localNodeService: this.#getOrDefaultLocalNodeService(),
            tunnelService: this.#getOrDefaultTunnelService(),
        });
        return this.#trustedService;
    }

    #getOrDefaultStreamService(): StreamService {
        this.#streamService ??= new StreamService({
            trustedService: this.#getOrDefaultTrustedService(),
        });
        return this.#streamService;
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

    withDefaultDiscoveryService(): DiscoveryService {
        if (this.#discoveryService !== undefined) {
            throw new Error("DiscoveryService is already set");
        }
        return this.#getOrDefaultDiscoveryService();
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
            discoveryService: this.#getOrDefaultDiscoveryService(),
            routingService: this.#getOrDefaultRoutingService(),
            rpcService: this.#getOrDefaultRpcService(),
            observerService: this.#getOrDefaultObserverService(),
            tunnelService: this.#getOrDefaultTunnelService(),
            trustedService: this.#getOrDefaultTrustedService(),
            streamService: this.#getOrDefaultStreamService(),
        });
    }
}
