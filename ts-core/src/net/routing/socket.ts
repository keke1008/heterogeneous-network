import { Err, Ok, Result } from "oxide.ts";
import { BufferWriter } from "@core/net/buffer";
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

    #includeLoopbackOnBroadcast?: boolean;

    constructor(args: {
        linkSocket: LinkSocket;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        routingService: RoutingService;
        maxFrameIdCacheSize: number;
        includeLoopbackOnBroadcast?: boolean;
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
        this.#includeLoopbackOnBroadcast = args.includeLoopbackOnBroadcast;
    }

    async #sendFrame(
        destination: Destination,
        data: Uint8Array,
        previousHop?: NodeId,
    ): Promise<Result<void, RoutingSendError | undefined>> {
        const unicast = async () => {
            const gatewayId = await this.#routingService.resolveGatewayNode(destination);
            if (gatewayId === undefined) {
                console.warn("failed to send routing frame: unreachable", destination);
                return Err({ type: "unreachable" } as const);
            }
            return this.#neighborSocket.send(gatewayId, data);
        };
        const broadcast = async () => {
            this.#neighborSocket.sendBroadcast(data, {
                ignoreNodeId: previousHop,
                includeLoopback: this.#includeLoopbackOnBroadcast,
            });
            return Ok(undefined);
        };

        if (destination.isUnicast()) {
            return unicast();
        }

        if (destination.isBroadcast()) {
            return broadcast();
        }

        // マルチキャストの場合
        const local = await this.#localNodeService.getSource();
        const isTargetLocal =
            local.matches(destination) || (await this.#localNodeService.isLocalNodeLikeId(destination.nodeId));
        if (isTargetLocal) {
            // 宛先に自分が含まれている場合はブロードキャストする
            return broadcast();
        } else {
            // マルチキャストの宛先へ中継する
            return unicast();
        }
    }

    async #handleReceivedFrame(neighborFrame: NeighborFrame): Promise<void> {
        const frameResult = ReceivedRoutingFrame.deserializeFromNeighborFrame(neighborFrame);
        if (frameResult.isErr()) {
            console.warn("failed to deserialize routing frame", frameResult.unwrapErr());
            return;
        }

        const frame = frameResult.unwrap();
        if (this.#frameIdCache.insertAndCheckContains(frame.frameId)) {
            return;
        }

        const previousHop = this.#neighborService.getNeighbor(frame.previousHop);
        if (previousHop === undefined) {
            console.warn("frame received from unknown neighbor", frame);
            return;
        }

        const local = await this.#localNodeService.getSource();
        if (local.matches(frame.destination) || frame.destination.nodeId.isLoopback()) {
            this.#onReceive?.(frame);
        }

        const repeat = frame.repeat();
        const payload = BufferWriter.serialize(RoutingFrame.serdeable.serializer(repeat)).expect(
            "Failed to serialize frame",
        );
        this.#sendFrame(frame.destination, payload, frame.previousHop);
    }

    onReceive(onReceive: (frame: RoutingFrame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive is already set");
        }
        this.#onReceive = onReceive;
    }

    async send(
        destination: Destination,
        payload: Uint8Array,
        ignoreNode?: NodeId,
    ): Promise<Result<void, RoutingSendError | undefined>> {
        const frame = new RoutingFrame({
            source: await this.#localNodeService.getSource(),
            destination,
            frameId: this.#frameIdCache.generateWithoutAdding(),
            payload,
        });

        const data = BufferWriter.serialize(RoutingFrame.serdeable.serializer(frame)).expect(
            "Failed to serialize frame",
        );
        return this.#sendFrame(destination, data, ignoreNode);
    }
}
