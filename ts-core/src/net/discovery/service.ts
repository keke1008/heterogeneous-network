import { Address, LinkService, Protocol } from "../link";
import { NeighborService, NeighborSocket } from "../neighbor";
import { Cost, LocalNodeService, NodeId } from "../node";
import { DiscoveryExtraType, DiscoveryFrame, DiscoveryRequestFrame, DiscoveryResponseFrame } from "./frame";
import { match } from "ts-pattern";
import { FrameIdCache } from "./frameId";
import { BufferReader, BufferWriter } from "../buffer";
import { LocalRequestStore } from "./request";
import { DiscoveryRequestCache } from "./cache";
import { unreachable } from "@core/types";

export interface ResolveMediaAddressesResult {
    gatewayId: NodeId;
    cost: Cost;
    extra: Address[];
}

export class DiscoveryService {
    #linkService: LinkService;
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
        this.#linkService = args.linkService;
        this.#localNodeService = args.localNodeService;
        this.#neighborService = args.neighborService;
        this.#neighborSocket = new NeighborSocket({
            linkSocket: args.linkService.open(Protocol.RoutingReactive),
            neighborService: args.neighborService,
        });

        this.#neighborSocket.onReceive(async (linkFrame) => {
            const result = DiscoveryFrame.deserialize(linkFrame.reader);
            if (result.isErr()) {
                console.warn("Failed to deserialize discovery frame", result.unwrapErr());
                return;
            }

            const frame = result.unwrap();
            if (this.#frameIdCache.has(frame.commonFields.frameId)) {
                this.#frameIdCache.add(frame.commonFields.frameId);
                return;
            }

            const senderNode = this.#neighborService.getNeighbor(frame.commonFields.senderId);
            if (senderNode === undefined) {
                return;
            }

            const { id, cost } = await this.#localNodeService.getInfo();
            const localCost = frame.commonFields.destinationId.equals(id) ? new Cost(0) : cost;
            this.#requestCache.addCache(frame, senderNode.edgeCost.add(localCost));

            if (frame instanceof DiscoveryRequestFrame) {
                this.#handleReceivedRequestFrame(frame);
            } else if (frame instanceof DiscoveryResponseFrame) {
                this.#handleReceivedResponseFrame(frame);
            } else {
                unreachable(frame);
            }
        });
    }

    async #sendFrame(destination: NodeId, frame: DiscoveryFrame) {
        const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
        frame.serialize(writer);
        const reader = new BufferReader(writer.unwrapBuffer());

        if (this.#neighborService.hasNeighbor(destination)) {
            await this.#neighborSocket.send(destination, reader);
            return;
        }

        const cache = this.#requestCache.getCache(destination);
        if (cache) {
            await this.#neighborSocket.send(cache.gatewayId, reader);
            return;
        }

        this.#neighborSocket.sendBroadcast(reader);
    }

    async #replyToRequestFrame(receivedFrame: DiscoveryRequestFrame) {
        const localId = await this.#localNodeService.getId();
        const extra = match(receivedFrame.extraType)
            .with(DiscoveryExtraType.None, () => undefined)
            .with(DiscoveryExtraType.ResolveAddresses, () => this.#linkService.getLocalAddresses())
            .run();
        const reply = DiscoveryResponseFrame.reply(receivedFrame, {
            frameId: receivedFrame.commonFields.frameId,
            localId,
            extra,
        });
        await this.#sendFrame(receivedFrame.commonFields.sourceId, reply);
    }

    async #repeatReceivedFrame(receivedFrame: DiscoveryFrame) {
        const { id: localId, cost: localCost } = await this.#localNodeService.getInfo();
        const senderNode = this.#neighborService.getNeighbor(receivedFrame.commonFields.senderId);
        if (senderNode === undefined) {
            return;
        }
        const repeat = receivedFrame.repeat({ localId, sourceLinkCost: senderNode.edgeCost, localCost });
        await this.#sendFrame(receivedFrame.commonFields.destinationId, repeat);
    }

    async #handleReceivedRequestFrame(frame: DiscoveryRequestFrame) {
        const localId = await this.#localNodeService.getId();
        if (frame.commonFields.destinationId.equals(localId)) {
            this.#replyToRequestFrame(frame);
        } else {
            this.#repeatReceivedFrame(frame);
        }
    }

    async #handleReceivedResponseFrame(frame: DiscoveryResponseFrame) {
        const localId = await this.#localNodeService.getId();
        if (frame.commonFields.destinationId.equals(localId)) {
            this.#requestStore.handleResponse(frame);
        } else {
            this.#repeatReceivedFrame(frame);
        }
    }

    async resolveGatewayNode(destinationId: NodeId): Promise<NodeId | undefined> {
        if (await this.#localNodeService.isLocalNodeLikeId(destinationId)) {
            return NodeId.loopback();
        }

        if (this.#neighborService.hasNeighbor(destinationId)) {
            return destinationId;
        }

        const cache = this.#requestCache.getCache(destinationId);
        if (cache?.gatewayId) {
            return cache.gatewayId;
        }

        const previous = this.#requestStore.getCurrentRequest(destinationId);
        if (previous?.extraType === DiscoveryExtraType.None) {
            const result = await previous.result;
            return result?.gatewayId;
        }

        const extraType = DiscoveryExtraType.None;
        const frame = DiscoveryRequestFrame.request({
            frameId: this.#frameIdCache.generateWithoutAdding(),
            destinationId,
            localId: await this.#localNodeService.getId(),
            extraType,
        });
        await this.#sendFrame(destinationId, frame);

        const result = await this.#requestStore.request(extraType, destinationId);
        return result?.gatewayId;
    }

    async resolveMediaAddresses(nodeId: NodeId): Promise<ResolveMediaAddressesResult | undefined> {
        const cache = this.#requestCache.getCache(nodeId);
        if (cache?.extra instanceof Array) {
            return { gatewayId: cache.gatewayId, cost: cache.cost, extra: cache.extra };
        }

        const previous = this.#requestStore.getCurrentRequest(nodeId);
        if (previous?.extraType === DiscoveryExtraType.ResolveAddresses) {
            const result = await previous.result;
            if (result?.extra instanceof Array) {
                return { gatewayId: result.gatewayId, cost: result.cost, extra: result.extra };
            }
        }

        const extraType = DiscoveryExtraType.ResolveAddresses;
        const frame = DiscoveryRequestFrame.request({
            frameId: this.#frameIdCache.generateWithoutAdding(),
            destinationId: nodeId,
            localId: await this.#localNodeService.getId(),
            extraType,
        });
        await this.#sendFrame(nodeId, frame);

        const result = await this.#requestStore.request(extraType, nodeId);
        if (result?.extra instanceof Array) {
            return { gatewayId: result.gatewayId, cost: result.cost, extra: result.extra };
        }
    }

    getCachedGatewayId(destinationId: NodeId): NodeId | undefined {
        const cache = this.#requestCache.getCache(destinationId);
        return cache?.gatewayId;
    }
}
