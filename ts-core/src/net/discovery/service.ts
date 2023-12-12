import { SingleListenerEventBroker } from "@core/event";
import { Address, LinkService, Protocol } from "../link";
import { NeighborService, NeighborSocket } from "../neighbor";
import { Cost, LocalNodeService, NodeId } from "../node";
import { DiscoveryExtraType, DiscoveryFrame, DiscoveryRequestFrame, DiscoveryResponseFrame } from "./frame";
import { P, match } from "ts-pattern";
import { FrameIdCache } from "./frameId";
import { BufferReader, BufferWriter } from "../buffer";
import { LocalRequestStore } from "./request";
import { DiscoveryRequestCache } from "./cache";

export class DiscoveryService {
    #linkService: LinkService;
    #localNodeService: LocalNodeService;
    #neighborService: NeighborService;

    #neighborSocket: NeighborSocket;
    #requestCache = new DiscoveryRequestCache();
    #requestStore = new LocalRequestStore();
    #frameIdCache = new FrameIdCache({});
    #onReceiveRepeatingRequest = new SingleListenerEventBroker<DiscoveryRequestFrame>();

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

        this.#neighborSocket.onReceive((linkFrame) => {
            const result = DiscoveryFrame.deserialize(linkFrame.reader);
            if (result.isErr()) {
                console.warn("Failed to deserialize discovery frame", result.unwrapErr());
                return;
            }

            const frame = result.unwrap();
            this.#requestCache.addCache(frame);

            match(result.unwrap())
                .with(P.instanceOf(DiscoveryRequestFrame), (frame) => this.#handleReceivedRequestFrame(frame))
                .with(P.instanceOf(DiscoveryResponseFrame), (frame) => this.#handleReceivedResponseFrame(frame))
                .exhaustive();
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

    async #handleReceivedRequestFrame(frame: DiscoveryRequestFrame) {
        const localId = await this.#localNodeService.getId();
        if (frame.commonFields.destinationId.equals(localId)) {
            this.replyResponseFrame(frame);
        } else {
            this.#onReceiveRepeatingRequest.emit(frame);
        }
    }

    async #handleReceivedResponseFrame(frame: DiscoveryResponseFrame) {
        const { id: localId, cost: localCost } = await this.#localNodeService.getInfo();
        if (frame.commonFields.destinationId.equals(localId)) {
            this.#requestStore.handleResponse(frame);
        } else {
            const senderNode = this.#neighborService.getNeighbor(frame.commonFields.senderId);
            if (senderNode === undefined) {
                return;
            }
            const repeat = new DiscoveryResponseFrame({
                frameId: this.#frameIdCache.generate(),
                totalCost: frame.commonFields.totalCost.add(senderNode.edgeCost).add(localCost),
                sourceId: frame.commonFields.sourceId,
                destinationId: frame.commonFields.destinationId,
                senderId: frame.commonFields.senderId,
                extra: frame.extra,
            });
            await this.#sendFrame(frame.commonFields.destinationId, repeat);
        }
    }

    onReceiveRepeatingRequest(handler: (request: DiscoveryRequestFrame) => void) {
        this.#onReceiveRepeatingRequest.listen(handler);
    }

    async repeatRequestFrame(destination: NodeId, receivedFrame: DiscoveryRequestFrame) {
        const localId = await this.#localNodeService.getId();
        const frame = new DiscoveryRequestFrame({
            frameId: this.#frameIdCache.generate(),
            totalCost: receivedFrame.commonFields.totalCost,
            sourceId: receivedFrame.commonFields.sourceId,
            destinationId: receivedFrame.commonFields.destinationId,
            senderId: localId,
            extraType: receivedFrame.extraType,
        });
        await this.#sendFrame(destination, frame);
    }

    async resolveGatewayNode(destinationId: NodeId): Promise<NodeId | undefined> {
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
        const frame = new DiscoveryRequestFrame({
            frameId: this.#frameIdCache.generate(),
            totalCost: new Cost(0),
            sourceId: destinationId,
            destinationId,
            senderId: destinationId,
            extraType,
        });
        await this.#sendFrame(destinationId, frame);

        const result = await this.#requestStore.request(extraType, destinationId);
        return result?.gatewayId;
    }

    async resolveMediaAddresses(nodeId: NodeId): Promise<Address[] | undefined> {
        const cache = this.#requestCache.getCache(nodeId);
        if (cache?.extra) {
            return cache.extra;
        }

        const previous = this.#requestStore.getCurrentRequest(nodeId);
        if (previous?.extraType === DiscoveryExtraType.ResolveAddresses) {
            const result = await previous.result;
            return result?.extra;
        }

        const extraType = DiscoveryExtraType.ResolveAddresses;
        const frame = new DiscoveryRequestFrame({
            frameId: this.#frameIdCache.generate(),
            totalCost: new Cost(0),
            sourceId: nodeId,
            destinationId: nodeId,
            senderId: nodeId,
            extraType,
        });
        await this.#sendFrame(nodeId, frame);

        const result = await this.#requestStore.request(extraType, nodeId);
        return result?.extra;
    }

    getCachedGatewayId(destinationId: NodeId): NodeId | undefined {
        const cache = this.#requestCache.getCache(destinationId);
        return cache?.gatewayId;
    }

    async replyResponseFrame(receivedFrame: DiscoveryRequestFrame) {
        const localId = await this.#localNodeService.getId();
        const extra = match(receivedFrame.extraType)
            .with(DiscoveryExtraType.None, () => undefined)
            .with(DiscoveryExtraType.ResolveAddresses, () => this.#linkService.getLocalAddresses())
            .run();
        const reply = new DiscoveryResponseFrame({
            frameId: receivedFrame.commonFields.frameId,
            totalCost: new Cost(0),
            sourceId: receivedFrame.commonFields.destinationId,
            destinationId: receivedFrame.commonFields.sourceId,
            senderId: localId,
            extra,
        });
        await this.#sendFrame(receivedFrame.commonFields.sourceId, reply);
    }
}
