import { Frame, Protocol, Address, LinkService, LinkSendError, FrameIdCache } from "@core/net/link";
import { NeighborEvent, NeighborService, NeighborNode, NeighborSocket } from "../neighbor";
import { Cost, NodeId } from "../../node";
import { RoutingCache } from "./cache";
import { RouteDiscoveryRequests } from "./discovery";
import { RouteDiscoveryFrameType, RouteDiscoveryFrame, deserializeFrame, serializeFrame } from "./frame";
import { Result } from "oxide.ts";
import { NotificationService } from "@core/net/notification";
import { match } from "ts-pattern";

export class ReactiveService {
    #notificationService: NotificationService;
    #neighborSocket: NeighborSocket;
    #neighborService: NeighborService;

    #localId: NodeId;
    #localCost: Cost;
    #cache: RoutingCache = new RoutingCache();
    #frameIdCache: FrameIdCache = new FrameIdCache({ maxCacheSize: 8 });

    #requests: RouteDiscoveryRequests = new RouteDiscoveryRequests();

    constructor(args: {
        notificationService: NotificationService;
        linkService: LinkService;
        localId: NodeId;
        localCost: Cost;
    }) {
        this.#notificationService = args.notificationService;

        const linkSocket = args.linkService.open(Protocol.RoutingReactive);
        this.#neighborService = new NeighborService({
            linkService: args.linkService,
            localId: args.localId,
            localCost: args.localCost,
        });
        this.#neighborService.onEvent((e) => this.#onNeighborEvent(e));

        this.#neighborSocket = new NeighborSocket({ linkSocket, neighborService: this.#neighborService });
        this.#neighborSocket.onReceive((frame) => this.#onFrameReceived(frame));

        this.#localId = args.localId;
        this.#localCost = args.localCost;
    }

    localId(): NodeId {
        return this.#localId;
    }

    localCost(): Cost {
        return this.#localCost;
    }

    neighborService(): NeighborService {
        return this.#neighborService;
    }

    #onFrameReceived(frame: Frame): void {
        const result_frame_ = deserializeFrame(frame.reader);
        if (result_frame_.isErr()) {
            return;
        }

        const frame_ = result_frame_.unwrap();
        this.#cache.add(frame_.sourceId, frame_.senderId);

        // 送信元がNeighborでない場合
        const senderNode = this.#neighborService.getNeighbor(frame_.senderId);
        if (senderNode === undefined) {
            return;
        }

        // 返信に備えてキャッシュに追加
        this.#cache.add(frame_.sourceId, frame_.senderId);

        // 探索対象が自分自身の場合
        if (frame_.targetId.equals(this.#localId)) {
            const totalCost = frame_.totalCost.add(senderNode.edgeCost);
            this.#requests.resolve(frame_.sourceId, frame_.senderId, totalCost);
            if (frame_.type === RouteDiscoveryFrameType.Request) {
                this.#replyRouteDiscovery(frame_, frame_.senderId);
            }
            return;
        }

        // 探索対象がキャッシュにある場合
        const gatewayId = this.#cache.get(frame_.targetId);
        if (gatewayId !== undefined) {
            const gatewayNode = this.#neighborService.getNeighbor(gatewayId);
            if (gatewayNode !== undefined) {
                this.#repeatUnicast(frame_, gatewayNode);
                return;
            }
        }

        // 探索対象がキャッシュにない場合
        this.#repeatBroadcast(frame_, senderNode);
    }

    #onNeighborEvent(event: NeighborEvent): void {
        match(event)
            .with({ type: "neighbor added" }, (e) => {
                this.#cache.add(e.neighborId, e.neighborId);
                this.#notificationService.notify({
                    type: "NeighborUpdated",
                    localCost: this.#localCost,
                    neighborId: e.neighborId,
                    neighborCost: e.neighborCost,
                    linkCost: e.linkCost,
                });
            })
            .with({ type: "neighbor removed" }, (e) => {
                this.#cache.remove(e.neighborId);
                this.#notificationService.notify({ type: "NeighborRemoved", nodeId: e.neighborId });
            })
            .exhaustive();
    }

    #replyRouteDiscovery(received: RouteDiscoveryFrame, senderId: NodeId): void {
        const frame = new RouteDiscoveryFrame(
            RouteDiscoveryFrameType.Reply,
            this.#frameIdCache.generate(),
            new Cost(0),
            this.#localId,
            received.sourceId,
            this.#localId,
        );
        this.#neighborSocket.send(senderId, serializeFrame(frame));
    }

    #repeatUnicast(received: RouteDiscoveryFrame, gatewayNode: NeighborNode): void {
        const frame = new RouteDiscoveryFrame(
            received.type,
            received.frameId,
            received.totalCost.add(gatewayNode.edgeCost).add(this.#localCost),
            received.sourceId,
            received.targetId,
            this.#localId,
        );
        this.#neighborSocket.send(gatewayNode.id, serializeFrame(frame));
    }

    #repeatBroadcast(received: RouteDiscoveryFrame, senderNode: NeighborNode): void {
        const frame = new RouteDiscoveryFrame(
            received.type,
            received.frameId,
            received.totalCost.add(senderNode.edgeCost).add(this.#localCost),
            received.sourceId,
            received.targetId,
            this.#localId,
        );
        const reader = serializeFrame(frame);
        this.#neighborSocket.sendBroadcast(reader, senderNode.id);
    }

    /**
     * 探索対象のノードIDを指定して探索を開始する。
     * 探索対象のノードIDが見つかった場合はゲートウェイのIDを返す。
     */
    async requestDiscovery(targetId: NodeId): Promise<NodeId | undefined> {
        const cached = this.#cache.get(targetId);
        if (cached !== undefined) {
            return cached;
        }

        const previous = this.#requests.get(targetId);
        if (previous !== undefined) {
            return previous;
        }

        const frame = new RouteDiscoveryFrame(
            RouteDiscoveryFrameType.Request,
            this.#frameIdCache.generate(),
            new Cost(0),
            this.#localId,
            targetId,
            this.#localId,
        );
        const reader = serializeFrame(frame);

        this.#neighborSocket.sendBroadcast(reader);
        return this.#requests.request(targetId);
    }

    async resolveAddress(id: NodeId): Promise<Address[]> {
        return this.#neighborService.resolveAddress(id);
    }

    requestHello(address: Address, edgeCost: Cost): Result<void, LinkSendError> {
        return this.#neighborService.sendHello(address, edgeCost);
    }

    requestGoodbye(destination: NodeId): Result<void, LinkSendError> {
        return this.#neighborService.sendGoodbye(destination);
    }

    getNeighborList(): NeighborNode[] {
        return this.#neighborService.getNeighbors();
    }
}
