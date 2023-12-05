import { AddressType, FrameHandler, LinkService } from "./link";
import { LocalNodeInfo } from "./node";
import { NotificationService } from "./notification";
import { ObserverService } from "./observer";
import { ReactiveService } from "./routing";
import { RpcService } from "./rpc";

export class NetFacade {
    #localNodeInfo = new LocalNodeInfo();

    #notificationService: NotificationService = new NotificationService();
    #linkService: LinkService = new LinkService();
    #routingService: ReactiveService;
    #rpcService: RpcService;
    #observerService: ObserverService;

    constructor() {
        this.#routingService = new ReactiveService({
            localNodeInfo: this.#localNodeInfo,
            notificationService: this.#notificationService,
            linkService: this.#linkService,
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

        this.#localNodeInfo.getInfo().then(({ cost }) => {
            this.#notificationService.notify({ type: "SelfUpdated", cost });
        });
    }

    localNodeInfo(): LocalNodeInfo {
        return this.#localNodeInfo;
    }

    addHandler(addressType: AddressType, handler: FrameHandler) {
        this.#linkService.addHandler(addressType, handler);

        const address = handler.address();
        if (address !== undefined) {
            this.#localNodeInfo.onAddressAdded(address);
        }
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
