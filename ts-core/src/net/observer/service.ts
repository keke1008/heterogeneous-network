import { NotificationService } from "../notification";
import { LinkService, Protocol } from "../link";
import { ReactiveService, RoutingSocket } from "../routing";
import { NodeService } from "./node";
import { ClientService } from "./client";
import { SinkService } from "./sink";
import { FrameType, deserializeObserverFrame } from "./frame";
import { match } from "ts-pattern";
import { NetworkUpdate } from "../node";
import { MAX_FRAME_ID_CACHE_SIZE } from "./constants";

export class ObserverService {
    #nodeService: NodeService;
    #sinkService?: SinkService;
    #clientService?: ClientService;
    #socket: RoutingSocket;

    constructor(args: {
        linkService: LinkService;
        reactiveService: ReactiveService;
        notificationService: NotificationService;
    }) {
        const linkSocket = args.linkService.open(Protocol.Observer);
        this.#socket = new RoutingSocket(linkSocket, args.reactiveService, MAX_FRAME_ID_CACHE_SIZE);

        this.#nodeService = new NodeService(args.notificationService, this.#socket);

        this.#socket.onReceive((frame) => {
            const deserialized = deserializeObserverFrame(frame.reader);
            if (deserialized.isErr()) {
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
        this.#sinkService = new SinkService(this.#socket);
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
