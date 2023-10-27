import { BufferReader, BufferWriter } from "../buffer";
import { LinkSocket } from "../link";
import { NodeId } from "./node";
import { ReactiveService } from "./reactive";

export type SendResult = { result: "success" } | { result: "failure"; reason: "unreachable" };

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
    #linkSocket: LinkSocket;
    #reactiveService: ReactiveService;
    #onReceive: ((frame: RoutingFrame) => void) | undefined;

    constructor(linkSocket: LinkSocket, reactiveService: ReactiveService) {
        this.#linkSocket = linkSocket;
        this.#reactiveService = reactiveService;
        this.#linkSocket.onReceive((frame) => {
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

    async send(destinationId: NodeId, reader: BufferReader): Promise<SendResult> {
        const routingFrame = new RoutingFrame({
            senderId: this.#reactiveService.selfId(),
            targetId: destinationId,
            reader,
        });

        const writer = new BufferWriter(new Uint8Array(routingFrame.serializedLength()));
        routingFrame.serialize(writer);

        const addr = await this.#reactiveService.resolveAddress(destinationId);
        if (addr.length === 0) {
            return { result: "failure", reason: "unreachable" };
        }

        this.#linkSocket.send(addr[0], new BufferReader(writer.unwrapBuffer()));
        return { result: "success" };
    }
}
