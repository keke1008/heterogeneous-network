import { Frame, Protocol, LinkService } from "@core/net/link";
import { NeighborService, NeighborNode, NeighborSocket } from "@core/net/neighbor";
import { Cost, LocalNodeService, NodeId } from "../../node";
import { RoutingCache } from "./cache";
import { RouteDiscoveryRequests } from "./discovery";
import { RouteDiscoveryFrameType, RouteDiscoveryFrame, deserializeFrame, serializeFrame } from "./frame";
import { FrameIdCache } from "../frameId";
import { RoutingService } from "../service";

export class ReactiveService implements RoutingService {
    #localNodeService: LocalNodeService;
    #neighborService: NeighborService;
    #neighborSocket: NeighborSocket;

    #cache: RoutingCache = new RoutingCache();
    #frameIdCache: FrameIdCache = new FrameIdCache({ maxCacheSize: 8 });

    #requests: RouteDiscoveryRequests = new RouteDiscoveryRequests();

    constructor(args: {
        linkService: LinkService;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
    }) {
        this.#localNodeService = args.localNodeService;
        this.#neighborService = args.neighborService;

        const linkSocket = args.linkService.open(Protocol.RoutingReactive);
        this.#neighborSocket = new NeighborSocket({ linkSocket, neighborService: this.#neighborService });
        this.#neighborSocket.onReceive((frame) => this.#onFrameReceived(frame));
    }

    async #onFrameReceived(frame: Frame): Promise<void> {
        const localId = await this.#localNodeService.getId();

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

    async #replyRouteDiscovery(received: RouteDiscoveryFrame, senderId: NodeId): Promise<void> {
        const localId = await this.#localNodeService.getId();

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
        const { id: localId, cost: localCost } = await this.#localNodeService.getInfo();
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
        const { id: localId, cost: localCost } = await this.#localNodeService.getInfo();
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
    async resolveGatewayNode(targetId: NodeId): Promise<NodeId | undefined> {
        if (this.#neighborService.hasNeighbor(targetId)) {
            return targetId;
        }

        const cached = this.#cache.get(targetId);
        if (cached !== undefined) {
            return cached;
        }

        const previous = this.#requests.get(targetId);
        if (previous !== undefined) {
            return previous;
        }

        const localId = await this.#localNodeService.getId();
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
}
