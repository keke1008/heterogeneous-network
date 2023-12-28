import { LinkService, Protocol } from "../link";
import { NeighborService, NeighborSocket } from "../neighbor";
import { NodeId } from "../node";
import { DiscoveryFrame, DiscoveryFrameType, ReceivedDiscoveryFrame } from "./frame";
import { FrameIdCache } from "./frameId";
import { BufferReader, BufferWriter } from "../buffer";
import { LocalRequestStore } from "./request";
import { DiscoveryRequestCache } from "./cache";
import { Destination } from "../node";
import { LocalNodeService } from "../local";

export class DiscoveryService {
    #localNodeService: LocalNodeService;
    #neighborService: NeighborService;

    #neighborSocket: NeighborSocket;
    #requestCache = new DiscoveryRequestCache();
    #requestStore = new LocalRequestStore();
    #frameIdCache = new FrameIdCache({});

    constructor(args: {
        linkService: LinkService;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
    }) {
        this.#localNodeService = args.localNodeService;
        this.#neighborService = args.neighborService;
        this.#neighborSocket = new NeighborSocket({
            linkSocket: args.linkService.open(Protocol.RoutingReactive),
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
            const senderNode = this.#neighborService.getNeighbor(frame.previousHop.nodeId);
            if (senderNode === undefined) {
                return;
            }

            // キャッシュに追加
            const { source: local, cost: localCost } = await this.#localNodeService.getInfo();
            const totalCost = frame.calculateTotalCost(senderNode.edgeCost, localCost);
            this.#requestCache.updateByReceivedFrame(frame, totalCost);

            if (local.matches(frame.destination())) {
                // 自分自身が探索対象の場合
                if (frame.type === DiscoveryFrameType.Request) {
                    // Requestであれば探索元に返信する
                    const frameId = this.#frameIdCache.generateWithoutAdding();
                    const reply = frame.reply({ frameId });
                    await this.#sendUnicastFrame(reply, frame.previousHop.nodeId);
                } else {
                    // ReplyであればRequestStoreに結果を登録する
                    this.#requestStore.handleResponse(frame);
                }
            } else {
                // �分自身が探索対象でない場合
                const cache = this.#requestCache.getCache(frame.destination());
                if (cache === undefined) {
                    // 探索対象がキャッシュにない場合，ブロードキャストする
                    const repeat = frame.repeat({ sourceLinkCost: senderNode.edgeCost, localCost });
                    await this.#sendBroadcastFrame(repeat, { ignore: frame.previousHop.nodeId });
                } else if (frame.type === DiscoveryFrameType.Request) {
                    // 探索対象がキャッシュにある場合，キャッシュからゲートウェイを取得して返信する
                    const frameId = this.#frameIdCache.generateWithoutAdding();
                    const reply = frame.replyByCache({ frameId, cache: cache.totalCost });
                    await this.#sendUnicastFrame(reply, cache.gateway);
                } else {
                    // Replyであれば中継する
                    const repeat = frame.repeat({ sourceLinkCost: senderNode.edgeCost, localCost });
                    await this.#sendUnicastFrame(repeat, cache.gateway);
                }
            }
        });
    }

    async #sendUnicastFrame(frame: DiscoveryFrame, nodeId: NodeId) {
        const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
        frame.serialize(writer);
        const reader = new BufferReader(writer.unwrapBuffer());
        await this.#neighborSocket.send(nodeId, reader);
    }

    async #sendBroadcastFrame(frame: DiscoveryFrame, args?: { ignore: NodeId }) {
        const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
        frame.serialize(writer);
        const reader = new BufferReader(writer.unwrapBuffer());
        this.#neighborSocket.sendBroadcast(reader, { ignoreNodeId: args?.ignore });
    }

    async resolveGatewayNode(destination: Destination): Promise<NodeId | undefined> {
        const destinationNodeId = destination.nodeId;
        if (destinationNodeId) {
            if (await this.#localNodeService.isLocalNodeLikeId(destinationNodeId)) {
                return NodeId.loopback();
            }

            if (this.#neighborService.hasNeighbor(destinationNodeId)) {
                return destinationNodeId;
            }
        }

        const cache = this.#requestCache.getCache(destination);
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

    getCachedGatewayId(destinationId: NodeId): NodeId | undefined {
        const cache = this.#requestCache.getCache(destinationId);
        return cache?.gateway;
    }
}
