import { BufferReader, BufferWriter } from "../buffer";
import { LinkSocket } from "../link";
import { NeighborSocket, SendResult } from "./neighbor";
import { NodeId } from "./node";
import { ReactiveService } from "./reactive";

export class RoutingFrame {
    senderId: NodeId;
    targetId: NodeId;
    reader: BufferReader;

    constructor(opts: { senderId: NodeId; targetId: NodeId; reader: BufferReader }) {
        this.senderId = opts.senderId;
        this.targetId = opts.targetId;
        this.reader = opts.reader;
    }

    static deserialize(reader: BufferReader): RoutingFrame {
        const senderId = NodeId.deserialize(reader);
        const targetId = NodeId.deserialize(reader);
        return new RoutingFrame({ senderId, targetId, reader });
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
            this.#onReceive?.(routingFrame);
        });
    }

    onReceive(onReceive: (frame: RoutingFrame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive is already set");
        }
        this.#onReceive = onReceive;
    }

    send(destinationId: NodeId, reader: BufferReader): SendResult {
        const routingFrame = new RoutingFrame({
            senderId: this.#reactiveService.selfId(),
            targetId: destinationId,
            reader,
        });

        const writer = new BufferWriter(new Uint8Array(routingFrame.serializedLength()));
        routingFrame.serialize(writer);

        return this.#neighborSocket.send(destinationId, new BufferReader(writer.unwrapBuffer()));
    }

    sendBroadcast(bodyReader: BufferReader, ignoreNode?: NodeId): void {
        const routingFrame = new RoutingFrame({
            senderId: this.#reactiveService.selfId(),
            targetId: NodeId.broadcast(),
            reader: bodyReader,
        });

        const writer = new BufferWriter(new Uint8Array(routingFrame.serializedLength()));
        routingFrame.serialize(writer);
        const reader = new BufferReader(writer.unwrapBuffer());
        this.#neighborSocket.sendBroadcast(reader, ignoreNode);
    }
}
