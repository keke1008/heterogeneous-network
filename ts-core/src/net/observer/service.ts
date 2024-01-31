import { NotificationService } from "../notification";
import { LinkService, Protocol } from "../link";
import { RoutingSocket } from "../routing";
import { NeighborService } from "../neighbor";
import { NodeService } from "./node";
import { ClientService } from "./client";
import { SinkService } from "./sink";
import { FrameType, NetworkUpdate, ObserverFrame } from "./frame";
import { match } from "ts-pattern";
import { NetworkTopologyUpdate } from "../node";
import { MAX_FRAME_ID_CACHE_SIZE, SOCKET_CONFIG } from "./constants";
import { RoutingService } from "../routing/service";
import { LocalNodeService } from "../local";
import { BufferReader } from "../buffer";

export class ObserverService {
    #localNodeService: LocalNodeService;
    #neighborService: NeighborService;
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
        this.#neighborService = args.neighborService;

        args.localNodeService.getInfo().then((info) => {
            args.notificationService.notify({
                type: "SelfUpdated",
                cost: info.cost,
                clusterId: info.clusterId,
            });
        });

        const linkSocket = args.linkService.open(Protocol.Observer);
        this.#socket = new RoutingSocket({
            linkSocket,
            config: SOCKET_CONFIG,
            localNodeService: args.localNodeService,
            neighborService: args.neighborService,
            routingService: args.routingService,
            maxFrameIdCacheSize: MAX_FRAME_ID_CACHE_SIZE,
            includeLoopbackOnBroadcast: true,
        });

        this.#nodeService = new NodeService({
            notificationService: args.notificationService,
            localNodeService: args.localNodeService,
            neighborService: args.neighborService,
            socket: this.#socket,
        });

        this.#socket.onReceive((frame) => {
            const deserialized = BufferReader.deserialize(ObserverFrame.serdeable.deserializer(), frame.payload);
            if (deserialized.isErr()) {
                console.warn("ObserverService: failed to deserialize frame", deserialized.unwrapErr());
                return;
            }
            match(deserialized.unwrap())
                .with({ frameType: FrameType.NodeSubscription }, (f) => {
                    this.#nodeService.dispatchReceivedFrame(frame.source, f);
                })
                .with({ frameType: FrameType.NodeNotification }, (f) => {
                    this.#sinkService?.dispatchReceivedFrame(frame.source, f);
                })
                .with({ frameType: FrameType.NodeSync }, (f) => {
                    this.#sinkService?.dispatchReceivedFrame(frame.source, f);
                })
                .with({ frameType: FrameType.NetworkSubscription }, (f) => {
                    this.#sinkService?.dispatchReceivedFrame(frame.source, f);
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
            neighborService: this.#neighborService,
        });
    }

    launchClientService(onNetworkUpdated: (updates: NetworkUpdate[]) => void) {
        if (this.#clientService) {
            throw new Error("Client service already launched");
        }
        this.#clientService = new ClientService({
            localNodeService: this.#localNodeService,
            neighborService: this.#neighborService,
            socket: this.#socket,
        });
        this.#clientService.onNetworkUpdated(onNetworkUpdated);
    }

    dumpNetworkStateAsUpdates(): NetworkTopologyUpdate[] {
        return this.#clientService?.dumpNetworkStateAsUpdates() ?? [];
    }

    close() {
        this.#sinkService?.close();
        this.#clientService?.close();
    }
}
