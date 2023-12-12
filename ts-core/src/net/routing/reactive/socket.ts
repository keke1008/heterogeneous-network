import { Err, Ok, Result } from "oxide.ts";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Frame, LinkSocket } from "@core/net/link";
import { NeighborSendError, NeighborSendErrorType, NeighborService, NeighborSocket } from "@core/net/neighbor";
import { LocalNodeService, NodeId } from "@core/net/node";
import { DeserializeResult } from "@core/serde";
import { FrameId, FrameIdCache } from "../frameId";
import { P, match } from "ts-pattern";
import { RoutingService } from "../service";

export const RoutingSendErrorType = NeighborSendErrorType;
export type RoutingSendErrorType = NeighborSendErrorType;
export type RoutingSendError = NeighborSendError;

export const RoutingBroadcastErrorType = { LocalAddressNotSet: "localAddress not set" } as const;
export type RoutingBroadcastErrorType = (typeof RoutingBroadcastErrorType)[keyof typeof RoutingBroadcastErrorType];
export type RoutingBroadcastError = { type: typeof RoutingBroadcastErrorType.LocalAddressNotSet };

export class UnicastRoutingFrameHeader {
    senderId: NodeId;
    targetId: NodeId;

    constructor(opts: { senderId: NodeId; targetId: NodeId }) {
        this.senderId = opts.senderId;
        this.targetId = opts.targetId;
    }

    get frameId(): undefined {
        return undefined;
    }

    serialize(writer: BufferWriter): void {
        this.senderId.serialize(writer);
        this.targetId.serialize(writer);
    }

    serializedLength(): number {
        return this.senderId.serializedLength() + this.targetId.serializedLength();
    }
}

export class BroadcastRoutingFrameHeader {
    senderId: NodeId;
    frameId: FrameId;

    constructor(opts: { senderId: NodeId; frameId: FrameId }) {
        this.senderId = opts.senderId;
        this.frameId = opts.frameId;
    }

    get targetId(): NodeId {
        return NodeId.broadcast();
    }

    serialize(writer: BufferWriter): void {
        this.senderId.serialize(writer);
        NodeId.broadcast().serialize(writer);
        this.frameId.serialize(writer);
    }

    serializedLength(): number {
        return (
            this.senderId.serializedLength() + NodeId.broadcast().serializedLength() + this.frameId.serializedLength()
        );
    }
}

export class RoutingFrame {
    header: UnicastRoutingFrameHeader | BroadcastRoutingFrameHeader;
    reader: BufferReader;

    private constructor(header: UnicastRoutingFrameHeader | BroadcastRoutingFrameHeader, reader: BufferReader) {
        this.header = header;
        this.reader = reader;
    }

    static unicast(opts: { senderId: NodeId; targetId: NodeId; reader: BufferReader }): RoutingFrame {
        return new RoutingFrame(
            new UnicastRoutingFrameHeader({ senderId: opts.senderId, targetId: opts.targetId }),
            opts.reader,
        );
    }

    static broadcast(opts: { senderId: NodeId; frameId: FrameId; reader: BufferReader }): RoutingFrame {
        return new RoutingFrame(
            new BroadcastRoutingFrameHeader({ senderId: opts.senderId, frameId: opts.frameId }),
            opts.reader,
        );
    }

    static deserialize(reader: BufferReader): DeserializeResult<RoutingFrame> {
        return NodeId.deserialize(reader).andThen((senderId) => {
            return NodeId.deserialize(reader).andThen((targetId) => {
                if (targetId.isBroadcast()) {
                    return FrameId.deserialize(reader).map((frameId) => {
                        return RoutingFrame.broadcast({ senderId, frameId, reader });
                    });
                } else {
                    return Ok(RoutingFrame.unicast({ senderId, targetId, reader }));
                }
            });
        });
    }

    serialize(writer: BufferWriter): void {
        this.header.serialize(writer);
        writer.writeBytes(this.reader.readRemaining());
    }

    serializedLength(): number {
        return this.header.serializedLength() + this.reader.remainingLength();
    }
}

export class ReactiveSocket {
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

    #handleReceivedFrame(frame: Frame): void {
        const routingFrame = RoutingFrame.deserialize(frame.reader);
        if (routingFrame.isErr()) {
            return;
        }

        match(routingFrame.unwrap())
            .with({ header: P.instanceOf(UnicastRoutingFrameHeader) }, async (frame) => {
                if (await this.#localNodeService.isLocalNodeLikeId(frame.header.targetId)) {
                    this.#onReceive?.(frame);
                } else {
                    const gatewayId = await this.#routingService.resolveGatewayNode(frame.header.targetId);
                    if (gatewayId !== undefined) {
                        this.#neighborSocket.send(gatewayId, frame.reader.initialized());
                    } else {
                        console.warn("failed to repeat routing frame: unreachable", frame);
                    }
                }
            })
            .with({ header: P.instanceOf(BroadcastRoutingFrameHeader) }, async (frame) => {
                if (this.#frameIdCache.has(frame.header.frameId)) {
                    return;
                }

                this.#frameIdCache.add(frame.header.frameId);
                this.#onReceive?.(frame);

                const localId = await this.#localNodeService.getId();
                if (!frame.header.senderId.equals(localId)) {
                    this.#neighborSocket.sendBroadcast(frame.reader.initialized(), frame.header.senderId);
                }
            })
            .exhaustive();
    }

    onReceive(onReceive: (frame: RoutingFrame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive is already set");
        }
        this.#onReceive = onReceive;
    }

    async #send(destinationId: NodeId, reader: BufferReader): Promise<Result<void, RoutingSendError>> {
        const gatewayId = await this.#routingService.resolveGatewayNode(destinationId);
        if (gatewayId === undefined) {
            return Err({ type: "unreachable" });
        }

        const localId = await this.#localNodeService.getId();
        const routingFrame = RoutingFrame.unicast({ senderId: localId, targetId: destinationId, reader });

        const writer = new BufferWriter(new Uint8Array(routingFrame.serializedLength()));
        routingFrame.serialize(writer);

        return this.#neighborSocket.send(gatewayId, new BufferReader(writer.unwrapBuffer()));
    }

    async #sendBroadcast(bodyReader: BufferReader, ignoreNode?: NodeId): Promise<Result<void, RoutingBroadcastError>> {
        const localId = await this.#localNodeService.getId();
        const routingFrame = RoutingFrame.broadcast({
            senderId: localId,
            frameId: this.#frameIdCache.generateWithoutAdding(),
            reader: bodyReader,
        });

        const writer = new BufferWriter(new Uint8Array(routingFrame.serializedLength()));
        routingFrame.serialize(writer);
        const reader = new BufferReader(writer.unwrapBuffer());
        this.#neighborSocket.sendBroadcast(reader, ignoreNode);

        return Ok(undefined);
    }

    async send(
        destinationId: NodeId,
        reader: BufferReader,
        ignoreNode?: NodeId,
    ): Promise<Result<void, RoutingSendError | RoutingBroadcastError>> {
        if (destinationId.isBroadcast()) {
            return this.#sendBroadcast(reader, ignoreNode);
        } else {
            return this.#send(destinationId, reader);
        }
    }
}
