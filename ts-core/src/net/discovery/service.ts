import { LinkService, Protocol } from "../link";
import { NeighborService, NeighborSocket } from "../neighbor";
import { LocalNodeService, NodeId } from "../node";
import { DiscoveryFrame, DiscoveryFrameType } from "./frame";
import { FrameIdCache } from "./frameId";
import { BufferReader, BufferWriter } from "../buffer";
import { LocalRequestStore } from "./request";
import { DiscoveryRequestCache } from "./cache";
import { unreachable } from "@core/types";
import { Destination } from "../node/destination";

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

            const senderNode = this.#neighborService.getNeighbor(frame.sender.nodeId);
            if (senderNode === undefined) {
                return;
            }

            this.#requestCache.update(frame);

            if (frame.type === DiscoveryFrameType.Request) {
                this.#handleReceivedRequestFrame(frame);
            } else if (frame.type === DiscoveryFrameType.Response) {
                this.#handleReceivedResponseFrame(frame);
            } else {
                unreachable(frame.type);
            }
        });
    }

    async #sendFrame(frame: DiscoveryFrame) {
        const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
        frame.serialize(writer);
        const reader = new BufferReader(writer.unwrapBuffer());

        const destinationNodeId = frame.destinationId();
        if (destinationNodeId && this.#neighborService.hasNeighbor(destinationNodeId)) {
            await this.#neighborSocket.send(destinationNodeId, reader);
            return;
        }

        const cache = this.#requestCache.getCache(frame.destination());
        if (cache) {
            await this.#neighborSocket.send(cache.gateway, reader);
            return;
        }

        this.#neighborSocket.sendBroadcast(reader);
    }

    async #repeatReceivedFrame(receivedFrame: DiscoveryFrame) {
        const { source: local, cost: localCost } = await this.#localNodeService.getInfo();
        const senderNode = this.#neighborService.getNeighbor(receivedFrame.sender.nodeId);
        if (senderNode === undefined) {
            return;
        }
        const repeat = receivedFrame.repeat({ local, sourceLinkCost: senderNode.edgeCost, localCost });
        await this.#sendFrame(repeat);
    }

    async #handleReceivedRequestFrame(frame: DiscoveryFrame) {
        const { source: local, clusterId: localClusterId } = await this.#localNodeService.getInfo();
        if (frame.target.matches(local.nodeId, localClusterId)) {
            const reply = frame.reply({ frameId: frame.frameId, local });
            await this.#sendFrame(reply);
        } else {
            this.#repeatReceivedFrame(frame);
        }
    }

    async #handleReceivedResponseFrame(frame: DiscoveryFrame) {
        const { id: localId, clusterId } = await this.#localNodeService.getInfo();
        if (frame.target.matches(localId, clusterId)) {
            this.#requestStore.handleResponse(frame);
        } else {
            this.#repeatReceivedFrame(frame);
        }
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
        await this.#sendFrame(frame);

        const result = await this.#requestStore.request(destination);
        return result?.gatewayId;
    }

    getCachedGatewayId(destinationId: NodeId): NodeId | undefined {
        const cache = this.#requestCache.getCache(destinationId);
        return cache?.gateway;
    }
}
