import { Err, Ok, Result } from "oxide.ts";
import { BufferWriter } from "@core/net/buffer";
import { LinkSocket } from "@core/net/link";
import {
    NeighborSendError,
    NeighborSendErrorType,
    NeighborService,
    NeighborSocket,
    NeighborFrame,
    NeighborSocketConfig,
} from "@core/net/neighbor";
import { Destination, NodeId } from "@core/net/node";
import { ReceivedRoutingFrame, RoutingFrame } from "./frame";
import { LocalNodeService } from "../local";
import { EventBroker } from "@core/event";
import { AsymmetricalFrameIdCache } from "./frameIdCache";
import { DiscoveryService } from "../discovery";

export const RoutingSendErrorType = NeighborSendErrorType;
export type RoutingSendErrorType = NeighborSendErrorType;
export type RoutingSendError = NeighborSendError;

export const RoutingBroadcastErrorType = { LocalAddressNotSet: "localAddress not set" } as const;
export type RoutingBroadcastErrorType = (typeof RoutingBroadcastErrorType)[keyof typeof RoutingBroadcastErrorType];

export class RoutingSocket {
    #neighborSocket: NeighborSocket;
    #localNodeService: LocalNodeService;
    #discoveryService: DiscoveryService;
    #onReceive: ((frame: RoutingFrame) => void) | undefined;
    #frameIdCache: AsymmetricalFrameIdCache;

    // 経由するフレームも含めて，フレームを受信したときに発火する
    #onFrameComming = new EventBroker<void>();

    #includeLoopbackOnBroadcast: boolean;

    constructor(args: {
        linkSocket: LinkSocket;
        config: NeighborSocketConfig;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        discoveryService: DiscoveryService;
        maxFrameIdCacheSize: number;
        includeLoopbackOnBroadcast: boolean;
    }) {
        this.#neighborSocket = new NeighborSocket({
            linkSocket: args.linkSocket,
            config: args.config,
            localNodeService: args.localNodeService,
            neighborService: args.neighborService,
        });
        this.#localNodeService = args.localNodeService;
        this.#discoveryService = args.discoveryService;
        this.#neighborSocket.onReceive((frame) => this.#handleReceivedFrame(frame));
        this.#frameIdCache = new AsymmetricalFrameIdCache({ maxCacheSize: args.maxFrameIdCacheSize });
        this.#includeLoopbackOnBroadcast = args.includeLoopbackOnBroadcast;
    }

    async maxPayloadLength(destination: Destination): Promise<number> {
        const headerLength = RoutingFrame.headerLength({
            source: await this.#localNodeService.getSource(),
            destination,
        });
        return this.#neighborSocket.maxPayloadLength() - headerLength;
    }

    async #sendFrame(
        destination: Destination,
        data: Uint8Array,
        previousHop?: NodeId,
    ): Promise<Result<void, RoutingSendError | undefined>> {
        const unicast = async () => {
            const gatewayId = await this.#discoveryService.resolveGatewayNode(destination);
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
        if (this.#frameIdCache.insertAndCheckContainsAsReceived(frame.frameId)) {
            return;
        }

        // フレームは受信または中継されるため，ここで発火する
        this.#onFrameComming.emit();

        const local = await this.#localNodeService.getSource();
        if (local.matches(frame.destination) || frame.destination.nodeId.isLoopback()) {
            this.#onReceive?.(frame);
        }

        if (this.#frameIdCache.containsAsSent(frame.frameId)) {
            return;
        }

        this.#sendFrame(frame.destination, neighborFrame.payload, frame.previousHop);
    }

    onReceive(onReceive: (frame: RoutingFrame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive is already set");
        }
        this.#onReceive = onReceive;
    }

    onFrameComming(callback: () => void): void {
        this.#onFrameComming.listen(callback);
    }

    async send(
        destination: Destination,
        payload: Uint8Array,
        ignoreNode?: NodeId,
    ): Promise<Result<void, RoutingSendError | undefined>> {
        const maxPayloadLength = await this.maxPayloadLength(destination);
        if (payload.length > maxPayloadLength) {
            throw new Error(`payload too large: ${payload.length} / ${maxPayloadLength}`);
        }

        const frame = new RoutingFrame({
            source: await this.#localNodeService.getSource(),
            destination,
            frameId: this.#frameIdCache.generate({ loopbackable: this.#includeLoopbackOnBroadcast }),
            payload,
        });

        const data = BufferWriter.serialize(RoutingFrame.serdeable.serializer(frame)).expect(
            "Failed to serialize frame",
        );
        return this.#sendFrame(destination, data, ignoreNode);
    }
}
