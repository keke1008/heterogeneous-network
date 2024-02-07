import { LinkService, Protocol } from "../link";
import { NeighborService, NeighborSocket } from "../neighbor";
import { Cost, NodeId } from "../node";
import { DiscoveryFrame, DiscoveryFrameType, ReceivedDiscoveryFrame } from "./frame";
import { FrameIdCache } from "./frameId";
import { BufferWriter } from "../buffer";
import { LocalRequestStore } from "./request";
import { DiscoveryRequestCache } from "./cache";
import { Destination } from "../node";
import { LocalNodeService } from "../local";
import { SOCKET_CONFIG } from "./constants";

export class AbortResolve {}
export class ResolveByDiscovery {}
export type ResolveResult = NodeId | AbortResolve | ResolveByDiscovery;

export interface RouteResolver {
    resolve(destination: Destination): Promise<ResolveResult>;
}

export class DiscoveryService {
    #localNodeService: LocalNodeService;
    #neighborService: NeighborService;

    #neighborSocket: NeighborSocket;
    #requestCache = new DiscoveryRequestCache();
    #requestStore = new LocalRequestStore();
    #frameIdCache = new FrameIdCache({});
    #routeResolvers: RouteResolver[] = [];

    constructor(args: {
        linkService: LinkService;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
    }) {
        this.#localNodeService = args.localNodeService;
        this.#neighborService = args.neighborService;
        this.#neighborSocket = new NeighborSocket({
            linkSocket: args.linkService.open(Protocol.RoutingReactive),
            config: SOCKET_CONFIG,
            localNodeService: args.localNodeService,
            neighborService: args.neighborService,
        });

        this.#neighborSocket.onReceive(async (neighborFrame) => {
            const result = ReceivedDiscoveryFrame.deserializeFromNeighborFrame(neighborFrame);
            if (result.isErr()) {
                console.warn("Failed to deserialize discovery frame", result.unwrapErr());
                return;
            }

            // 既にキャッシュにある（受信済み）場合は無視する
            const frame = result.unwrap();
            if (this.#frameIdCache.insertAndCheckContains(frame.frameId)) {
                return;
            }

            // 送信元がNeighborでない場合は無視する
            const senderNode = this.#neighborService.getNeighbor(frame.previousHop);
            if (senderNode === undefined) {
                return;
            }

            // キャッシュに追加
            const { source: local, cost: localCost } = await this.#localNodeService.getInfo();
            const totalCost = frame.calculateTotalCost(senderNode.linkCost, localCost);
            this.#requestCache.updateByReceivedFrame(frame, totalCost);

            if (local.matches(frame.destination())) {
                // 自分自身が探索対象の場合
                if (frame.type === DiscoveryFrameType.Request) {
                    // Requestであれば探索元に返信する
                    const frameId = this.#frameIdCache.generateWithoutAdding();
                    const reply = frame.reply({ frameId, totalCost: new Cost(0) });
                    await this.#sendUnicastFrame(reply, frame.previousHop);
                } else {
                    // ReplyであればRequestStoreに結果を登録する
                    this.#requestStore.handleResponse(frame);
                }
                return;
            }

            // 静的なルート解決を試みる
            for (const resolver of this.#routeResolvers) {
                const gateway = await resolver.resolve(frame.destination());
                if (gateway instanceof AbortResolve) {
                    return;
                }
                if (gateway instanceof ResolveByDiscovery) {
                    continue;
                }

                const repeat = frame.repeat({ sourceLinkCost: senderNode.linkCost, localCost });
                await this.#sendUnicastFrame(repeat, gateway);
                return;
            }

            const node_id_cache = this.#requestCache.getCacheByNodeId(frame.destination().nodeId);
            if (node_id_cache !== undefined) {
                if (frame.type === DiscoveryFrameType.Request) {
                    // キャッシュからゲートウェイを取得して返信する
                    const frameId = this.#frameIdCache.generateWithoutAdding();
                    const reply = frame.replyByCache({ frameId, cache: node_id_cache.totalCost });
                    await this.#sendUnicastFrame(reply, node_id_cache.gateway);
                } else {
                    // Replyであれば中継する
                    const repeat = frame.repeat({ sourceLinkCost: senderNode.linkCost, localCost });
                    await this.#sendUnicastFrame(repeat, node_id_cache.gateway);
                }
                return;
            }

            const linkCost = args.neighborService.getNeighbor(frame.destination().nodeId)?.linkCost;
            if (linkCost !== undefined) {
                if (frame.type === DiscoveryFrameType.Request) {
                    const frameId = this.#frameIdCache.generateWithoutAdding();
                    const totalCost = new Cost(0).add(linkCost).add(localCost);
                    const reply = frame.reply({ frameId, totalCost });
                    await this.#sendUnicastFrame(reply, frame.previousHop);
                } else {
                    const repeat = frame.repeat({ sourceLinkCost: senderNode.linkCost, localCost });
                    await this.#sendUnicastFrame(repeat, frame.destination().nodeId);
                }
                return;
            }

            const cluster_id_cache = this.#requestCache.getCacheByClusterId(frame.destination().clusterId);
            if (cluster_id_cache !== undefined) {
                if (!local.clusterId.isNoCluster() && local.clusterId.equals(frame.destination().clusterId)) {
                    // 自ノードとクラスタIDが一致する場合，ブロードキャスト
                    const repeat = frame.repeat({ sourceLinkCost: senderNode.linkCost, localCost });
                    await this.#sendBroadcastFrame(repeat, { ignore: frame.previousHop });
                } else {
                    // クラスタIDが異なる場合，キャッシュのゲートウェイに中継する
                    const repeat = frame.repeat({ sourceLinkCost: senderNode.linkCost, localCost });
                    await this.#sendUnicastFrame(repeat, cluster_id_cache.gateway);
                }
                return;
            }

            // キャッシュにない場合，ブロードキャストする
            const repeat = frame.repeat({ sourceLinkCost: senderNode.linkCost, localCost });
            await this.#sendBroadcastFrame(repeat, { ignore: frame.previousHop });
        });
    }

    async #sendUnicastFrame(frame: DiscoveryFrame, nodeId: NodeId) {
        const payload = BufferWriter.serialize(DiscoveryFrame.serdeable.serializer(frame)).expect(
            "failed to serialize frame",
        );
        await this.#neighborSocket.send(nodeId, payload);
    }

    async #sendBroadcastFrame(frame: DiscoveryFrame, args?: { ignore: NodeId }) {
        const payload = BufferWriter.serialize(DiscoveryFrame.serdeable.serializer(frame)).expect(
            "failed to serialize frame",
        );
        this.#neighborSocket.sendBroadcast(payload, { ignoreNodeId: args?.ignore });
    }

    async resolveGatewayNode(destination: Destination): Promise<NodeId | undefined> {
        if (destination.isBroadcast()) {
            return NodeId.broadcast();
        }

        const destinationNodeId = destination.nodeId;
        if (await this.#localNodeService.isLocalNodeLikeId(destinationNodeId)) {
            return NodeId.loopback();
        }

        if (this.#neighborService.hasNeighbor(destinationNodeId)) {
            return destinationNodeId;
        }

        for (const resolver of this.#routeResolvers) {
            const gateway = await resolver.resolve(destination);
            if (gateway instanceof AbortResolve) {
                return;
            }
            if (gateway instanceof ResolveByDiscovery) {
                continue;
            }

            return gateway;
        }

        const cache = this.#requestCache.getCacheByNodeId(destination.nodeId);
        if (cache?.gateway) {
            return cache.gateway;
        }

        const previous = this.#requestStore.getCurrentRequest(destination);
        if (previous !== undefined) {
            return (await previous.result)?.gatewayId;
        }

        const frame = DiscoveryFrame.request({
            frameId: this.#frameIdCache.generateWithoutAdding(),
            destination,
            local: await this.#localNodeService.getSource(),
        });
        await this.#sendBroadcastFrame(frame);

        const result = await this.#requestStore.request(destination);
        return result?.gatewayId;
    }

    injectRouteResolver(resolver: RouteResolver) {
        this.#routeResolvers.push(resolver);
    }
}
