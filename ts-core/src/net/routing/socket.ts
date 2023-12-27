import { Err, Ok, Result } from "oxide.ts";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { LinkSocket } from "@core/net/link";
import {
    NeighborSendError,
    NeighborSendErrorType,
    NeighborService,
    NeighborSocket,
    NeighborFrame,
} from "@core/net/neighbor";
import { Destination, NodeId } from "@core/net/node";
import { FrameIdCache } from "@core/net/discovery";
import { RoutingService } from "./service";
import { ReceivedRoutingFrame, RoutingFrame } from "./frame";
import { LocalNodeService } from "../local";

export const RoutingSendErrorType = NeighborSendErrorType;
export type RoutingSendErrorType = NeighborSendErrorType;
export type RoutingSendError = NeighborSendError;

export const RoutingBroadcastErrorType = { LocalAddressNotSet: "localAddress not set" } as const;
export type RoutingBroadcastErrorType = (typeof RoutingBroadcastErrorType)[keyof typeof RoutingBroadcastErrorType];

export class RoutingSocket {
    #neighborSocket: NeighborSocket;
    #localNodeService: LocalNodeService;
    #neighborService: NeighborService;
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
            localNodeService: args.localNodeService,
            neighborService: args.neighborService,
        });
        this.#neighborService = args.neighborService;
        this.#localNodeService = args.localNodeService;
        this.#routingService = args.routingService;
        this.#neighborSocket.onReceive((frame) => this.#handleReceivedFrame(frame));
        this.#frameIdCache = new FrameIdCache({ maxCacheSize: args.maxFrameIdCacheSize });
    }

    async #sendFrame(
        destination: Destination,
        reader: BufferReader,
        previousHop?: NodeId,
    ): Promise<Result<void, RoutingSendError | undefined>> {
        const local = await this.#localNodeService.getSource();

        const isTargetLocal = local.matches(destination) || destination.nodeId?.isLoopback();
        if (!isTargetLocal) {
            const gatewayId = await this.#routingService.resolveGatewayNode(destination);
            if (gatewayId === undefined) {
                console.warn("failed to send routing frame: unreachable", destination);
                return Err({ type: "unreachable" });
            }

            return this.#neighborSocket.send(gatewayId, reader);
        }

        if (!destination.isUnicast()) {
            this.#neighborSocket.sendBroadcast(reader, previousHop);
        }
        return Ok(undefined);
    }

    async #handleReceivedFrame(neighborFrame: NeighborFrame): Promise<void> {
        const frameResult = ReceivedRoutingFrame.deserializeFromNeighborFrame(neighborFrame);
        if (frameResult.isErr()) {
            console.warn("failed to deserialize routing frame", frameResult.unwrapErr());
            return;
        }

        const frame = frameResult.unwrap();
        if (this.#frameIdCache.insert_and_check_contains(frame.frameId)) {
            return;
        }

        const previousHop = this.#neighborService.getNeighbor(frame.previousHop);
        if (previousHop === undefined) {
            console.warn("frame received from unknown neighbor", frame);
            return;
        }

        const local = await this.#localNodeService.getSource();
        if (local.matches(frame.destination) || frame.destination.nodeId?.isLoopback()) {
            this.#onReceive?.(frame);
        }

        const repeat = frame.repeat();
        const writer = new BufferWriter(new Uint8Array(repeat.serializedLength()));
        repeat.serialize(writer);
        this.#sendFrame(frame.destination, new BufferReader(writer.unwrapBuffer()), frame.previousHop);
    }

    onReceive(onReceive: (frame: RoutingFrame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive is already set");
        }
        this.#onReceive = onReceive;
    }

    async send(
        destination: Destination,
        reader: BufferReader,
        ignoreNode?: NodeId,
    ): Promise<Result<void, RoutingSendError | undefined>> {
        const frame = new RoutingFrame({
            source: await this.#localNodeService.getSource(),
            destination,
            frameId: this.#frameIdCache.generateWithoutAdding(),
            reader,
        });

        const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
        frame.serialize(writer);
        return this.#sendFrame(destination, new BufferReader(writer.unwrapBuffer()), ignoreNode);
    }
}
