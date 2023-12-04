import { Frame, Protocol, Address, LinkService, LinkSendError } from "@core/net/link";
import { NeighborEvent, NeighborService, NeighborNode, NeighborSocket } from "@core/net/neighbor";
import { Cost, LocalNodeInfo, NodeId } from "../../node";
import { RoutingCache } from "./cache";
import { RouteDiscoveryRequests } from "./discovery";
import { RouteDiscoveryFrameType, RouteDiscoveryFrame, deserializeFrame, serializeFrame } from "./frame";
import { Result } from "oxide.ts";
import { NotificationService } from "@core/net/notification";
import { match } from "ts-pattern";
import { FrameIdCache } from "../frameId";

export class ReactiveService {
    #localNodeInfo: LocalNodeInfo;

    #notificationService: NotificationService;
    #neighborSocket: NeighborSocket;
    #neighborService: NeighborService;

    #cache: RoutingCache = new RoutingCache();
    #frameIdCache: FrameIdCache = new FrameIdCache({ maxCacheSize: 8 });

    #requests: RouteDiscoveryRequests = new RouteDiscoveryRequests();

    constructor(args: {
        notificationService: NotificationService;
        linkService: LinkService;
        localNodeInfo: LocalNodeInfo;
    }) {
        this.#localNodeInfo = args.localNodeInfo;

        this.#notificationService = args.notificationService;
        this.#neighborService = new NeighborService({
            linkService: args.linkService,
            localNodeInfo: args.localNodeInfo,
        });
        this.#neighborService.onEvent((e) => this.#onNeighborEvent(e));

        const linkSocket = args.linkService.open(Protocol.RoutingReactive);
        this.#neighborSocket = new NeighborSocket({ linkSocket, neighborService: this.#neighborService });
        this.#neighborSocket.onReceive((frame) => this.#onFrameReceived(frame));
    }

    localNodeInfo(): LocalNodeInfo {
        return this.#localNodeInfo;
    }

    neighborService(): NeighborService {
        return this.#neighborService;
    }

    async #onFrameReceived(frame: Frame): Promise<void> {
        const localId = await this.#localNodeInfo.getId();

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
        if (frame_.targetId.equals(localId)) {
            const totalCost = frame_.totalCost.add(senderNode.edgeCost);
            this.#requests.resolve(frame_.sourceId, frame_.senderId, totalCost);
            if (frame_.type === RouteDiscoveryFrameType.Request) {
                await this.#replyRouteDiscovery(frame_, frame_.senderId);
            }
            return;
        }

        // 探索対象がキャッシュにある場合
        const gatewayId = this.#cache.get(frame_.targetId);
        if (gatewayId !== undefined) {
            const gatewayNode = this.#neighborService.getNeighbor(gatewayId);
            if (gatewayNode !== undefined) {
                await this.#repeatUnicast(frame_, gatewayNode);
                return;
            }
        }

        // 探索対象がキャッシュにない場合
        await this.#repeatBroadcast(frame_, senderNode);
    }

    #onNeighborEvent(event: NeighborEvent): void {
        match(event)
            .with({ type: "neighbor added" }, async (e) => {
                this.#cache.add(e.neighborId, e.neighborId);

                const localCost = await this.#localNodeInfo.getCost();

                this.#notificationService.notify({
                    type: "NeighborUpdated",
                    localCost,
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

    async #replyRouteDiscovery(received: RouteDiscoveryFrame, senderId: NodeId): Promise<void> {
        const localId = await this.#localNodeInfo.getId();

        const frame = new RouteDiscoveryFrame(
            RouteDiscoveryFrameType.Reply,
            this.#frameIdCache.generate(),
            new Cost(0),
            localId,
            received.sourceId,
            localId,
        );
        this.#neighborSocket.send(senderId, serializeFrame(frame));
    }

    async #repeatUnicast(received: RouteDiscoveryFrame, gatewayNode: NeighborNode): Promise<void> {
        const { id: localId, cost: localCost } = await this.#localNodeInfo.getInfo();
        const frame = new RouteDiscoveryFrame(
            received.type,
            received.frameId,
            received.totalCost.add(gatewayNode.edgeCost).add(localCost),
            received.sourceId,
            received.targetId,
            localId,
        );
        this.#neighborSocket.send(gatewayNode.id, serializeFrame(frame));
    }

    async #repeatBroadcast(received: RouteDiscoveryFrame, senderNode: NeighborNode): Promise<void> {
        const { id: localId, cost: localCost } = await this.#localNodeInfo.getInfo();
        const frame = new RouteDiscoveryFrame(
            received.type,
            received.frameId,
            received.totalCost.add(senderNode.edgeCost).add(localCost),
            received.sourceId,
            received.targetId,
            localId,
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

        const localId = await this.#localNodeInfo.getId();
        const frame = new RouteDiscoveryFrame(
            RouteDiscoveryFrameType.Request,
            this.#frameIdCache.generate(),
            new Cost(0),
            localId,
            targetId,
            localId,
        );
        const reader = serializeFrame(frame);

        this.#neighborSocket.sendBroadcast(reader);
        return this.#requests.request(targetId);
    }

    async resolveAddress(id: NodeId): Promise<Address[]> {
        return this.#neighborService.resolveAddress(id);
    }

    requestHello(address: Address, edgeCost: Cost): Promise<Result<void, LinkSendError>> {
        return this.#neighborService.sendHello(address, edgeCost);
    }

    requestGoodbye(destination: NodeId): Promise<Result<void, LinkSendError>> {
        return this.#neighborService.sendGoodbye(destination);
    }

    getNeighborList(): NeighborNode[] {
        return this.#neighborService.getNeighbors();
    }
}
