import { Ok, Result } from "oxide.ts";
import { BufferReader, BufferWriter } from "../buffer";
import { Frame, LinkSocket } from "../link";
import { NeighborSendError, NeighborSendErrorType, NeighborSocket } from "@core/net/neighbor";
import { LocalNodeInfo, NodeId } from "@core/net/node";
import { ReactiveService } from "./reactive";
import { DeserializeResult } from "@core/serde";
import { FrameId, FrameIdCache } from "./frameId";
import { P, match } from "ts-pattern";

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

export class RoutingSocket {
    #neighborSocket: NeighborSocket;
    #reactiveService: ReactiveService;
    #onReceive: ((frame: RoutingFrame) => void) | undefined;
    #frameIdCache: FrameIdCache;

    constructor(linkSocket: LinkSocket, reactiveService: ReactiveService, maxFrameIdCacheSize: number) {
        this.#neighborSocket = new NeighborSocket({ linkSocket, neighborService: reactiveService.neighborService() });
        this.#reactiveService = reactiveService;
        this.#neighborSocket.onReceive((frame) => this.#handleReceivedFrame(frame));
        this.#frameIdCache = new FrameIdCache({ maxCacheSize: maxFrameIdCacheSize });
    }

    #handleReceivedFrame(frame: Frame): void {
        const routingFrame = RoutingFrame.deserialize(frame.reader);
        if (routingFrame.isErr()) {
            return;
        }

        match(routingFrame.unwrap())
            .with({ header: P.instanceOf(UnicastRoutingFrameHeader) }, async (frame) => {
                const localId = await this.#reactiveService.localNodeInfo().getId();
                if (frame.header.targetId.equals(localId)) {
                    this.#onReceive?.(frame);
                } else {
                    const id = await this.#reactiveService.requestDiscovery(frame.header.targetId);
                    if (id !== undefined) {
                        this.#neighborSocket.send(id, frame.reader.initialized());
                    }
                }
            })
            .with({ header: P.instanceOf(BroadcastRoutingFrameHeader) }, (frame) => {
                if (!this.#frameIdCache.has(frame.header.frameId)) {
                    this.#frameIdCache.add(frame.header.frameId);
                    this.#onReceive?.(frame);
                    this.#neighborSocket.sendBroadcast(frame.reader.initialized(), frame.header.senderId);
                }
            })
            .exhaustive();
    }

    localNodeInfo(): LocalNodeInfo {
        return this.#reactiveService.localNodeInfo();
    }

    onReceive(onReceive: (frame: RoutingFrame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive is already set");
        }
        this.#onReceive = onReceive;
    }

    async send(destinationId: NodeId, reader: BufferReader): Promise<Result<void, RoutingSendError>> {
        const localId = await this.#reactiveService.localNodeInfo().getId();
        const routingFrame = RoutingFrame.unicast({ senderId: localId, targetId: destinationId, reader });

        const writer = new BufferWriter(new Uint8Array(routingFrame.serializedLength()));
        routingFrame.serialize(writer);

        return this.#neighborSocket.send(destinationId, new BufferReader(writer.unwrapBuffer()));
    }

    async sendBroadcast(bodyReader: BufferReader, ignoreNode?: NodeId): Promise<Result<void, RoutingBroadcastError>> {
        const localId = await this.#reactiveService.localNodeInfo().getId();
        const routingFrame = RoutingFrame.broadcast({
            senderId: localId,
            frameId: this.#frameIdCache.generate(),
            reader: bodyReader,
        });

        const writer = new BufferWriter(new Uint8Array(routingFrame.serializedLength()));
        routingFrame.serialize(writer);
        const reader = new BufferReader(writer.unwrapBuffer());
        this.#neighborSocket.sendBroadcast(reader, ignoreNode);

        return Ok(undefined);
    }
}
