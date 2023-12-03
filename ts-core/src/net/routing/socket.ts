import { Err, Ok, Result } from "oxide.ts";
import { BufferReader, BufferWriter } from "../buffer";
import { LinkSocket } from "../link";
import { NeighborSendError, NeighborSendErrorType, NeighborSocket } from "@core/net/neighbor";
import { NodeId } from "@core/net/node";
import { ReactiveService } from "./reactive";
import { DeserializeResult } from "@core/serde";

export const RoutingSendErrorType = NeighborSendErrorType;
export type RoutingSendErrorType = NeighborSendErrorType;
export type RoutingSendError = NeighborSendError;

export const RoutingBroadcastErrorType = { LocalAddressNotSet: "localAddress not set" } as const;
export type RoutingBroadcastErrorType = (typeof RoutingBroadcastErrorType)[keyof typeof RoutingBroadcastErrorType];
export type RoutingBroadcastError = { type: typeof RoutingBroadcastErrorType.LocalAddressNotSet };

export class RoutingFrame {
    senderId: NodeId;
    targetId: NodeId;
    reader: BufferReader;

    constructor(opts: { senderId: NodeId; targetId: NodeId; reader: BufferReader }) {
        this.senderId = opts.senderId;
        this.targetId = opts.targetId;
        this.reader = opts.reader;
    }

    static deserialize(reader: BufferReader): DeserializeResult<RoutingFrame> {
        return NodeId.deserialize(reader).andThen((senderId) => {
            return NodeId.deserialize(reader).map((targetId) => {
                return new RoutingFrame({ senderId, targetId, reader });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        this.senderId.serialize(writer);
        this.targetId.serialize(writer);
        writer.writeBytes(this.reader.readRemaining());
    }

    serializedLength(): number {
        return this.senderId.serializedLength() + this.targetId.serializedLength() + this.reader.remainingLength();
    }
}

export class RoutingSocket {
    #neighborSocket: NeighborSocket;
    #reactiveService: ReactiveService;
    #onReceive: ((frame: RoutingFrame) => void) | undefined;

    constructor(linkSocket: LinkSocket, reactiveService: ReactiveService) {
        this.#neighborSocket = new NeighborSocket({ linkSocket, neighborService: reactiveService.neighborService() });
        this.#reactiveService = reactiveService;
        this.#neighborSocket.onReceive((frame) => {
            const routingFrame = RoutingFrame.deserialize(frame.reader);
            if (routingFrame.isOk()) {
                this.#onReceive?.(routingFrame.unwrap());
            }
        });
    }

    localId(): NodeId {
        return this.#reactiveService.localId();
    }

    onReceive(onReceive: (frame: RoutingFrame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive is already set");
        }
        this.#onReceive = onReceive;
    }

    send(destinationId: NodeId, reader: BufferReader): Result<void, RoutingSendError> {
        const senderId = this.#reactiveService.localId();
        if (senderId === undefined) {
            return Err({ type: RoutingSendErrorType.LocalAddressNotSet });
        }
        const routingFrame = new RoutingFrame({ senderId, targetId: destinationId, reader });

        const writer = new BufferWriter(new Uint8Array(routingFrame.serializedLength()));
        routingFrame.serialize(writer);

        return this.#neighborSocket.send(destinationId, new BufferReader(writer.unwrapBuffer()));
    }

    sendBroadcast(bodyReader: BufferReader, ignoreNode?: NodeId): Result<void, RoutingBroadcastError> {
        const senderId = this.#reactiveService.localId();
        if (senderId === undefined) {
            return Err({ type: RoutingBroadcastErrorType.LocalAddressNotSet });
        }
        const routingFrame = new RoutingFrame({ senderId, targetId: NodeId.broadcast(), reader: bodyReader });

        const writer = new BufferWriter(new Uint8Array(routingFrame.serializedLength()));
        routingFrame.serialize(writer);
        const reader = new BufferReader(writer.unwrapBuffer());
        this.#neighborSocket.sendBroadcast(reader, ignoreNode);

        return Ok(undefined);
    }
}
