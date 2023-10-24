import { Frame, Protocol, Address, LinkService, LinkSocket } from "@core/net/link";
import { NeighborEvent, NeighborService, NeighborNode } from "../neighbor";
import { Cost, NodeId } from "../node";
import { RoutingCache } from "./cache";
import { RouteDiscoveryRequests } from "./discovery";
import { RouteDiscoveryFrameType, RouteDiscoveryFrame, deserializeFrame, serializeFrame } from "./frame";
import { FrameIdManager } from "./frameId";

export class ReactiveService {
    #socket: LinkSocket;
    #selfId: NodeId;
    #selfCost: Cost;
    #cache: RoutingCache = new RoutingCache();
    #frameId: FrameIdManager = new FrameIdManager();
    #requests: RouteDiscoveryRequests = new RouteDiscoveryRequests();
    #neighborService: NeighborService;

    constructor(linkService: LinkService, selfId: NodeId, selfCost: Cost) {
        this.#socket = linkService.open(Protocol.RoutingReactive);
        this.#socket.onReceive((frame) => this.#onFrameReceived(frame));

        this.#selfId = selfId;
        this.#selfCost = selfCost;
        this.#neighborService = new NeighborService(linkService, selfId);
        this.#neighborService.onEvent((e) => this.#onNeighborEvent(e));
    }

    #onFrameReceived(frame: Frame): void {
        const frame_ = deserializeFrame(frame.reader);
        this.#cache.add(frame_.sourceId, frame_.senderId);

        // 送信元がNeighborでない場合
        const senderNode = this.#neighborService.getNeighbor(frame_.senderId);
        if (senderNode === undefined) {
            return;
        }

        // 探索対象が自分自身の場合
        if (frame_.targetId.equals(this.#selfId)) {
            const totalCost = frame_.totalCost.add(senderNode.edgeCost);
            this.#requests.resolve(frame_.sourceId, frame_.senderId, totalCost);
            if (frame_.type === RouteDiscoveryFrameType.Request) {
                this.#replyRouteDiscovery(frame_, frame.sender);
            }
            return;
        }

        // 探索対象がキャッシュにある場合
        const gatewayId = this.#cache.get(frame_.targetId);
        if (gatewayId !== undefined) {
            const gatewayNode = this.#neighborService.getNeighbor(gatewayId);
            if (gatewayNode !== undefined) {
                this.#repeatUnicast(frame_, gatewayNode, senderNode);
                return;
            }
        }

        // 探索対象がキャッシュにない場合
        this.#repeatBroadcast(frame_, senderNode, frame.sender);
    }

    #onNeighborEvent(event: NeighborEvent): void {
        if (event.type === "neighbor removed") {
            this.#cache.remove(event.peerId);
        }
    }

    #replyRouteDiscovery(received: RouteDiscoveryFrame, senderAddress: Address): void {
        const frame = new RouteDiscoveryFrame(
            RouteDiscoveryFrameType.Reply,
            this.#frameId.next(),
            new Cost(0),
            this.#selfId,
            received.sourceId,
            this.#selfId,
        );
        this.#socket.send(senderAddress, serializeFrame(frame));
    }

    #repeatUnicast(received: RouteDiscoveryFrame, gatewayNode: NeighborNode, senderNode: NeighborNode): void {
        const frame = new RouteDiscoveryFrame(
            received.type,
            received.frameId,
            received.totalCost.add(gatewayNode.edgeCost).add(senderNode.edgeCost).add(this.#selfCost),
            received.sourceId,
            received.targetId,
            this.#selfId,
        );
        this.#socket.send(gatewayNode.addresses[0], serializeFrame(frame));
    }

    #repeatBroadcast(received: RouteDiscoveryFrame, senderNode: NeighborNode, senderAddress: Address): void {
        const frame = new RouteDiscoveryFrame(
            received.type,
            received.frameId,
            received.totalCost.add(senderNode.edgeCost).add(this.#selfCost),
            received.sourceId,
            received.targetId,
            this.#selfId,
        );
        const buffer = serializeFrame(frame);

        this.#neighborService
            .getNeighbors()
            .flatMap((neighbors) => neighbors.addresses.slice(0, 1))
            .filter((address) => !address.equals(senderAddress))
            .forEach((address) => {
                this.#socket.send(address, buffer.initialized());
            });
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
            this.#frameId.next(),
            new Cost(0),
            this.#selfId,
            targetId,
            this.#selfId,
        );
        const buffer = serializeFrame(frame);

        this.#neighborService
            .getNeighbors()
            .flatMap((neighbors) => neighbors.addresses.slice(0, 1))
            .forEach((address) => {
                this.#socket.send(address, buffer.initialized());
            });

        return this.#requests.request(targetId);
    }

    async resolveAddress(id: NodeId): Promise<Address[]> {
        return this.#neighborService.resolveAddress(id);
    }

    requestHello(address: Address, edgeCost: Cost): void {
        return this.#neighborService.sendHello(address, edgeCost);
    }

    requestGoodbye(destination: NodeId): void {
        return this.#neighborService.sendGoodbye(destination);
    }
}
