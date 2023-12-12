import { Protocol, LinkService, Frame } from "@core/net/link";
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
import { ResolveWithMediaAddressesResult, RoutingService } from "../service";

export type CustomFrameHandler = (args: {
    routeCache: RoutingCache;
    frameIdCache: FrameIdCache;
    requests: RouteDiscoveryRequests;
}) => (frame: RouteDiscoveryFrame) => Promise<void>;

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
        customFrameHandler?: CustomFrameHandler;
    }) {
        this.#linkService = args.linkService;
        this.#localNodeService = args.localNodeService;
        this.#neighborService = args.neighborService;

        const linkSocket = args.linkService.open(Protocol.RoutingReactive);
        this.#neighborSocket = new NeighborSocket({ linkSocket, neighborService: this.#neighborService });

        const customFrameHandler = args.customFrameHandler?.({
            routeCache: this.#cache,
            frameIdCache: this.#frameIdCache,
            requests: this.#requests,
        });
        const frameHandler = customFrameHandler ?? this.#defaultFrameHandler.bind(this);
        this.#neighborSocket.onReceive((frame) => this.#commonFrameHandler(frame, frameHandler));
    }

    async #commonFrameHandler(frame: Frame, handler: (frame: RouteDiscoveryFrame) => Promise<void>): Promise<void> {
        const result = deserializeFrame(frame.reader);
        if (result.isErr()) {
            console.warn(`Failed to deserialize frame: ${result.unwrapErr()}`);
            return;
        }

        // 返信に備えてキャッシュに追加
        const f = result.unwrap();
        this.#cache.addByFrame(f);

        handler(f);
    }

    async #defaultFrameHandler(frame: RouteDiscoveryFrame): Promise<void> {
        const localId = await this.#localNodeService.getId();

        // 送信元がNeighborでない場合
        const senderNode = this.#neighborService.getNeighbor(frame.senderId);
        if (senderNode === undefined) {
            return;
        }

        // 探索対象が自分自身の場合
        if (frame.targetId.equals(localId)) {
            this.#requests.resolve(frame, senderNode.edgeCost);
            if (frame.type === RouteDiscoveryFrameType.Request) {
                const reply = await frame.reply(this.#linkService, this.#localNodeService, this.#frameIdCache);
                await this.#neighborSocket.send(frame.senderId, serializeFrame(reply));
            }
            return;
        }

        // 探索対象がキャッシュにある場合
        const cache = this.#cache.get(frame.targetId);
        if (cache !== undefined) {
            const gatewayNode = this.#neighborService.getNeighbor(cache.gatewayId);
            if (gatewayNode !== undefined) {
                const repeat = await frame.repeat(this.#localNodeService, gatewayNode.edgeCost);
                await this.#neighborSocket.send(frame.senderId, serializeFrame(repeat));
                return;
            }
        }

        // 探索対象がキャッシュにない場合
        const repeat = await frame.repeat(this.#localNodeService, senderNode.edgeCost);
        this.#neighborSocket.sendBroadcast(serializeFrame(repeat), senderNode.id);
    }

    async resolveCommon(targetId: NodeId, extra?: ExtraSpecifier) {
        const cache = this.#cache.get(targetId);
        if (cache !== undefined) {
            return cache;
        }

        const previous = await this.#requests.get(targetId);
        if (previous !== undefined) {
            return previous;
        }

        const frame = await RouteDiscoveryFrame.request(this.#frameIdCache, this.#localNodeService, targetId, extra);
        const reader = serializeFrame(frame);
        this.#neighborSocket.sendBroadcast(reader);
        return await this.#requests.request(targetId);
    }

    /**
     * 探索対象のノードIDを指定して探索を開始する。
     * 探索対象のノードIDが見つかった場合はゲートウェイのIDを返す。
     */
    async resolveGatewayNode(targetId: NodeId): Promise<NodeId | undefined> {
        if (this.#neighborService.hasNeighbor(targetId)) {
            return targetId;
        }

        return (await this.resolveCommon(targetId))?.gatewayId;
    }

    async resolveGatewayNodeWithMediaAddresses(targetId: NodeId): Promise<ResolveWithMediaAddressesResult | undefined> {
        const request = await this.resolveCommon(targetId, ExtraSpecifier.MediaAddress);
        return request === undefined ? undefined : { gatewayId: request.gatewayId, extra: request.extra };
    }
}
