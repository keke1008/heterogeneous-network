import { NotificationService } from "../notification";
import { LinkService, Protocol } from "../link";
import { RoutingSocket } from "../routing";
import { NeighborService } from "../neighbor";
import { NodeService } from "./node";
import { ClientService } from "./client";
import { SinkService } from "./sink";
import { FrameType, deserializeObserverFrame } from "./frame";
import { match } from "ts-pattern";
import { LocalNodeService, NetworkUpdate } from "../node";
import { MAX_FRAME_ID_CACHE_SIZE } from "./constants";
import { RoutingService } from "../routing/service";

export class ObserverService {
    #localNodeService: LocalNodeService;
    #nodeService: NodeService;
    #sinkService?: SinkService;
    #clientService?: ClientService;
    #socket: RoutingSocket;

    constructor(args: {
        linkService: LinkService;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        routingService: RoutingService;
        notificationService: NotificationService;
    }) {
        this.#localNodeService = args.localNodeService;
        args.localNodeService.getCost().then((cost) => {
            args.notificationService.notify({ type: "SelfUpdated", cost });
        });

        const linkSocket = args.linkService.open(Protocol.Observer);
        this.#socket = new RoutingSocket({
            linkSocket,
            localNodeService: args.localNodeService,
            neighborService: args.neighborService,
            routingService: args.routingService,
            maxFrameIdCacheSize: MAX_FRAME_ID_CACHE_SIZE,
        });

        this.#nodeService = new NodeService({
            localNodeService: args.localNodeService,
            notificationService: args.notificationService,
            socket: this.#socket,
        });

        this.#socket.onReceive((frame) => {
            const deserialized = deserializeObserverFrame(frame.reader);
            if (deserialized.isErr()) {
                console.warn("ObserverService: failed to deserialize frame", deserialized.unwrapErr());
                return;
            }
            match(deserialized.unwrap())
                .with({ frameType: FrameType.NodeSubscription }, (f) => {
                    this.#nodeService.dispatchReceivedFrame(frame.header.senderId, f);
                })
                .with({ frameType: FrameType.NodeNotification }, (f) => {
                    this.#sinkService?.dispatchReceivedFrame(frame.header.senderId, f);
                })
                .with({ frameType: FrameType.NetworkSubscription }, (f) => {
                    this.#sinkService?.dispatchReceivedFrame(frame.header.senderId, f);
                })
                .with({ frameType: FrameType.NetworkNotification }, (f) => {
                    this.#clientService?.dispatchReceivedFrame(f);
                })
                .otherwise((frame) => {
                    throw new Error(`Unknown frame type: ${frame.frameType}`);
                });
        });
    }

    launchSinkService() {
        if (this.#sinkService) {
            throw new Error("Sink service already launched");
        }
        this.#sinkService = new SinkService({
            socket: this.#socket,
            localNodeService: this.#localNodeService,
        });
    }

    launchClientService(onNetworkUpdated: (updates: NetworkUpdate[]) => void) {
        if (this.#clientService) {
            throw new Error("Client service already launched");
        }
        this.#clientService = new ClientService(this.#socket);
        this.#clientService.onNetworkUpdated(onNetworkUpdated);
    }

    dumpNetworkStateAsUpdates(): NetworkUpdate[] {
        return this.#clientService?.dumpNetworkStateAsUpdates() ?? [];
    }

    close() {
        this.#sinkService?.close();
        this.#clientService?.close();
    }
}
