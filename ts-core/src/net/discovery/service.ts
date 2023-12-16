import { LinkService, Protocol } from "../link";
import { NeighborService, NeighborSocket } from "../neighbor";
import { Cost, LocalNodeService, NodeId } from "../node";
import { DiscoveryFrame, DiscoveryFrameType } from "./frame";
import { FrameIdCache } from "./frameId";
import { BufferReader, BufferWriter } from "../buffer";
import { LocalRequestStore } from "./request";
import { DiscoveryRequestCache } from "./cache";
import { unreachable } from "@core/types";

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
            neighborService: args.neighborService,
        });

        this.#neighborSocket.onReceive(async (linkFrame) => {
            const result = DiscoveryFrame.deserialize(linkFrame.reader);
            if (result.isErr()) {
                console.warn("Failed to deserialize discovery frame", result.unwrapErr());
                return;
            }

            const frame = result.unwrap();
            if (this.#frameIdCache.has(frame.frameId)) {
                this.#frameIdCache.add(frame.frameId);
                return;
            }

            const senderNode = this.#neighborService.getNeighbor(frame.senderId);
            if (senderNode === undefined) {
                return;
            }

            const { id, cost } = await this.#localNodeService.getInfo();
            const localCost = frame.destinationId.equals(id) ? new Cost(0) : cost;
            this.#requestCache.addCache(frame, senderNode.edgeCost.add(localCost));

            if (frame.type === DiscoveryFrameType.Request) {
                this.#handleReceivedRequestFrame(frame);
            } else if (frame.type === DiscoveryFrameType.Response) {
                this.#handleReceivedResponseFrame(frame);
            } else {
                unreachable(frame.type);
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

    async #replyToFrame(receivedFrame: DiscoveryFrame) {
        const localId = await this.#localNodeService.getId();
        const reply = receivedFrame.reply({ frameId: receivedFrame.frameId, localId });
        await this.#sendFrame(receivedFrame.sourceId, reply);
    }

    async #repeatReceivedFrame(receivedFrame: DiscoveryFrame) {
        const { id: localId, cost: localCost } = await this.#localNodeService.getInfo();
        const senderNode = this.#neighborService.getNeighbor(receivedFrame.senderId);
        if (senderNode === undefined) {
            return;
        }
        const repeat = receivedFrame.repeat({ localId, sourceLinkCost: senderNode.edgeCost, localCost });
        await this.#sendFrame(receivedFrame.destinationId, repeat);
    }

    async #handleReceivedRequestFrame(frame: DiscoveryFrame) {
        const localId = await this.#localNodeService.getId();
        if (frame.destinationId.equals(localId)) {
            this.#replyToFrame(frame);
        } else {
            this.#repeatReceivedFrame(frame);
        }
    }

    async #handleReceivedResponseFrame(frame: DiscoveryFrame) {
        const localId = await this.#localNodeService.getId();
        if (frame.destinationId.equals(localId)) {
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
        if (previous !== undefined) {
            return (await previous.result)?.gatewayId;
        }

        const frame = DiscoveryFrame.request({
            frameId: this.#frameIdCache.generateWithoutAdding(),
            destinationId,
            localId: await this.#localNodeService.getId(),
        });
        await this.#sendFrame(destinationId, frame);

        const result = await this.#requestStore.request(destinationId);
        return result?.gatewayId;
    }

    getCachedGatewayId(destinationId: NodeId): NodeId | undefined {
        const cache = this.#requestCache.getCache(destinationId);
        return cache?.gatewayId;
    }
}
