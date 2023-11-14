import { AddressType, FrameHandler, LinkService } from "./link";
import { NotificationService } from "./notification";
import { Cost, ReactiveService } from "./routing";
import { RpcService } from "./rpc";

export class NetFacade {
    #notificationService: NotificationService = new NotificationService();

    #linkService: LinkService = new LinkService({
        notificationService: this.#notificationService,
    });

    #routingService: ReactiveService = new ReactiveService({
        notificationService: this.#notificationService,
        linkService: this.#linkService,
        selfCost: new Cost(0),
    });

    #rpcService: RpcService = new RpcService({
        notificationService: this.#notificationService,
        linkService: this.#linkService,
        reactiveService: this.#routingService,
    });

    #observerService: ObserverService = new ObserverService({
        notificationService: this.#notificationService,
        linkService: this.#linkService,
        reactiveService: this.#routingService,
    });

    addHandler(addressType: AddressType, handler: FrameHandler) {
        this.#linkService.addHandler(addressType, handler);
    }

    routing(): ReactiveService {
        return this.#routingService;
    }

    rpc(): RpcService {
        return this.#rpcService;
    }

    notification(): NotificationService {
        return this.#notificationService;
    }
}
