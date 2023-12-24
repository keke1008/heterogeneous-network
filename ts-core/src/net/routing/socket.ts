import { Err, Ok, Result } from "oxide.ts";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Frame, LinkSocket } from "@core/net/link";
import { NeighborSendError, NeighborSendErrorType, NeighborService, NeighborSocket } from "@core/net/neighbor";
import { Destination, NodeId } from "@core/net/node";
import { FrameIdCache } from "@core/net/discovery";
import { RoutingService } from "./service";
import { RoutingFrame } from "./frame";
import { LocalNodeService } from "../local";

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

        const { source: local } = await this.#localNodeService.getInfo();
        return local.matches(destination);
    }

    async #handleReceivedFrame(frame: Frame): Promise<void> {
        const routingFrameResult = RoutingFrame.deserialize(frame.reader);
        if (routingFrameResult.isErr()) {
            console.warn("failed to deserialize routing frame", routingFrameResult.unwrapErr());
            return;
        }

        const frameId = routingFrameResult.unwrap().frameId;
        if (this.#frameIdCache.insert_and_check_contains(frameId)) {
            return;
        }

        const routingFrame = routingFrameResult.unwrap();
        if (await this.#isSelfNodeTarget(routingFrame.destination)) {
            this.#onReceive?.(routingFrame);
            if (!routingFrame.destination.isUnicast()) {
                this.#neighborSocket.sendBroadcast(routingFrame.reader.initialized(), routingFrame.source.nodeId);
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
        const source = await this.#localNodeService.getSource();
        const frameId = this.#frameIdCache.generateWithoutAdding();
        const routingFrame = new RoutingFrame({ source, destination, frameId, reader });

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
