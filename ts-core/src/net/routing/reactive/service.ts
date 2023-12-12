import { Frame, Protocol, LinkService, Address } from "@core/net/link";
import { NeighborService, NeighborSocket } from "@core/net/neighbor";
import { LocalNodeService, NodeId } from "../../node";
import { RoutingCache } from "./cache";
import { RouteDiscoveryRequests } from "./discovery";
import {
    RouteDiscoveryFrameType,
    RouteDiscoveryFrame,
    deserializeFrame,
    serializeFrame,
    ExtraSpecifier,
} from "./frame";
import { FrameIdCache } from "../frameId";
import { RoutingService } from "../service";

export class ReactiveService implements RoutingService {
    #linkService: LinkService;
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
        this.#linkService = args.linkService;
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

        // 送信元がNeighborでない場合
        const senderNode = this.#neighborService.getNeighbor(frame_.senderId);
        if (senderNode === undefined) {
            return;
        }

        // 返信に備えてキャッシュに追加
        this.#cache.addByFrame(frame_);

        // 探索対象が自分自身の場合
        if (frame_.targetId.equals(localId)) {
            this.#requests.resolve(frame_, senderNode.edgeCost);
            if (frame_.type === RouteDiscoveryFrameType.Request) {
                const reply = await frame_.reply(this.#linkService, this.#localNodeService, this.#frameIdCache);
                await this.#neighborSocket.send(frame_.senderId, serializeFrame(reply));
            }
            return;
        }

        // 探索対象がキャッシュにある場合
        const cache = this.#cache.get(frame_.targetId);
        if (cache !== undefined) {
            const gatewayNode = this.#neighborService.getNeighbor(cache.gatewayId);
            if (gatewayNode !== undefined) {
                const repeat = await frame_.repeat(this.#localNodeService, gatewayNode.edgeCost);
                await this.#neighborSocket.send(frame_.senderId, serializeFrame(repeat));
                return;
            }
        }

        // 探索対象がキャッシュにない場合
        const repeat = await frame_.repeat(this.#localNodeService, senderNode.edgeCost);
        this.#neighborSocket.sendBroadcast(serializeFrame(repeat), senderNode.id);
    }

    /**
     * 探索対象のノードIDを指定して探索を開始する。
     * 探索対象のノードIDが見つかった場合はゲートウェイのIDを返す。
     */
    async resolveGatewayNode(targetId: NodeId): Promise<NodeId | undefined> {
        if (this.#neighborService.hasNeighbor(targetId)) {
            return targetId;
        }

        const cache = this.#cache.get(targetId);
        if (cache !== undefined) {
            return cache.gatewayId;
        }

        const previous = await this.#requests.get(targetId);
        if (previous !== undefined) {
            return previous.gatewayId;
        }

        const frame = await RouteDiscoveryFrame.request(this.#frameIdCache, this.#localNodeService, targetId);
        const reader = serializeFrame(frame);

        this.#neighborSocket.sendBroadcast(reader);
        const request = await this.#requests.request(targetId);
        return request?.gatewayId;
    }

    async resolveMediaAddresses(targetId: NodeId): Promise<Address[] | undefined> {
        const cache = this.#cache.get(targetId);
        if (cache !== undefined) {
            return cache.extra;
        }

        const previous = await this.#requests.get(targetId);
        if (previous !== undefined) {
            return previous.extra;
        }

        const frame = await RouteDiscoveryFrame.request(
            this.#frameIdCache,
            this.#localNodeService,
            targetId,
            ExtraSpecifier.MediaAddress,
        );

        const reader = serializeFrame(frame);
        this.#neighborSocket.sendBroadcast(reader);
        const request = await this.#requests.request(targetId);
        return request?.extra;
    }
}
