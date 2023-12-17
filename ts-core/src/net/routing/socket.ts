import { Err, Ok, Result } from "oxide.ts";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Frame, LinkSocket } from "@core/net/link";
import { NeighborSendError, NeighborSendErrorType, NeighborService, NeighborSocket } from "@core/net/neighbor";
import { LocalNodeService, NodeId } from "@core/net/node";
import { Destination, FrameIdCache } from "@core/net/discovery";
import { RoutingService } from "./service";
import { RoutingFrame } from "./frame";

export const RoutingSendErrorType = NeighborSendErrorType;
export type RoutingSendErrorType = NeighborSendErrorType;
export type RoutingSendError = NeighborSendError;

export const RoutingBroadcastErrorType = { LocalAddressNotSet: "localAddress not set" } as const;
export type RoutingBroadcastErrorType = (typeof RoutingBroadcastErrorType)[keyof typeof RoutingBroadcastErrorType];

export class RoutingSocket {
    #neighborSocket: NeighborSocket;
    #localNodeService: LocalNodeService;
    #routingService: RoutingService;
    #onReceive: ((frame: RoutingFrame) => void) | undefined;
    #frameIdCache: FrameIdCache;

    constructor(args: {
        linkSocket: LinkSocket;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        routingService: RoutingService;
        maxFrameIdCacheSize: number;
    }) {
        this.#neighborSocket = new NeighborSocket({
            linkSocket: args.linkSocket,
            neighborService: args.neighborService,
        });
        this.#localNodeService = args.localNodeService;
        this.#routingService = args.routingService;
        this.#neighborSocket.onReceive((frame) => this.#handleReceivedFrame(frame));
        this.#frameIdCache = new FrameIdCache({ maxCacheSize: args.maxFrameIdCacheSize });
    }

    async #isSelfNodeTarget(destination: Destination): Promise<boolean> {
        const destinationId = destination.nodeId;
        if (destinationId && (await this.#localNodeService.isLocalNodeLikeId(destinationId))) {
            return true;
        }

        const { id: localId, clusterId } = await this.#localNodeService.getInfo();
        return destination.matches(localId, clusterId);
    }

    async #handleReceivedFrame(frame: Frame): Promise<void> {
        const routingFrameResult = RoutingFrame.deserialize(frame.reader);
        if (routingFrameResult.isErr()) {
            return;
        }

        const frameId = routingFrameResult.unwrap().frameId;
        if (this.#frameIdCache.has(frameId)) {
            return;
        }
        this.#frameIdCache.add(frameId);

        const routingFrame = routingFrameResult.unwrap();
        if (await this.#isSelfNodeTarget(routingFrame.destination)) {
            this.#onReceive?.(routingFrame);
            if (routingFrame.destination.hasOnlyClusterId()) {
                this.#neighborSocket.sendBroadcast(routingFrame.reader.initialized(), routingFrame.sourceId);
            }
        } else {
            const gatewayId = await this.#routingService.resolveGatewayNode(routingFrame.destination);
            if (gatewayId !== undefined) {
                this.#neighborSocket.send(gatewayId, routingFrame.reader.initialized());
            } else {
                console.warn("failed to repeat routing frame: unreachable", routingFrame);
            }
        }
    }

    onReceive(onReceive: (frame: RoutingFrame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive is already set");
        }
        this.#onReceive = onReceive;
    }

    async #createRoutingFrameReader(destination: Destination, reader: BufferReader): Promise<BufferReader> {
        const localId = await this.#localNodeService.getId();
        const frameId = this.#frameIdCache.generateWithoutAdding();
        const routingFrame = new RoutingFrame({ sourceId: localId, destination, frameId, reader });

        const writer = new BufferWriter(new Uint8Array(routingFrame.serializedLength()));
        routingFrame.serialize(writer);

        return new BufferReader(writer.unwrapBuffer());
    }

    async send(
        destination: Destination,
        reader: BufferReader,
        ignoreNode?: NodeId,
    ): Promise<Result<void, RoutingSendError | undefined>> {
        if (destination.isBroadcast()) {
            const frameReader = await this.#createRoutingFrameReader(Destination.broadcast(), reader);
            this.#neighborSocket.sendBroadcast(frameReader, ignoreNode);
            return Ok(undefined);
        } else {
            const gatewayId = await this.#routingService.resolveGatewayNode(destination);
            if (gatewayId === undefined) {
                return Err({ type: "unreachable" });
            }

            const frameReader = await this.#createRoutingFrameReader(destination, reader);
            return this.#neighborSocket.send(gatewayId, frameReader);
        }
    }
}
