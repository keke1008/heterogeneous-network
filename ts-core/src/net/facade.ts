import { AddressType, FrameHandler, LinkService } from "./link";
import { NotificationService } from "./notification/service";
import { Cost, ReactiveService } from "./routing";
import { RpcService } from "./rpc";

export class NetFacade {
    #linkService: LinkService = new LinkService();

    #routingService: ReactiveService = new ReactiveService({
        linkService: this.#linkService,
        selfCost: new Cost(0),
    });

    #rpcService: RpcService = new RpcService({
        linkService: this.#linkService,
        reactiveService: this.#routingService,
    });

    #notificationService: NotificationService = new NotificationService({
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
