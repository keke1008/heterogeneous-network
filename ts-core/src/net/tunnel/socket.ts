import { TunnelPortId } from "./port";
import { Destination } from "../node";
import { ReceivedTunnelFrame, TunnelFrame } from "./frame";
import { Result } from "oxide.ts";
import { NeighborSendError } from "../neighbor";
import { Receiver } from "@core/channel";

export interface FrameSender {
    send(frame: TunnelFrame): Promise<Result<void, NeighborSendError | undefined>>;
    maxPayloadLength(): Promise<number>;
}

export class TunnelSocket {
    #localPortId: TunnelPortId;
    #destination: Destination;
    #destinationPortId: TunnelPortId;

    #sender: FrameSender;
    #receiver: Receiver<ReceivedTunnelFrame>;

    constructor(args: {
        localPortId: TunnelPortId;
        destination: Destination;
        destinationPortId: TunnelPortId;
        sender: FrameSender;
        receiver: Receiver<ReceivedTunnelFrame>;
    }) {
        this.#localPortId = args.localPortId;
        this.#destination = args.destination;
        this.#destinationPortId = args.destinationPortId;
        this.#sender = args.sender;
        this.#receiver = args.receiver;
    }

    get localPortId(): TunnelPortId {
        return this.#localPortId;
    }

    get destination(): Destination {
        return this.#destination;
    }

    get destinationPortId(): TunnelPortId {
        return this.#destinationPortId;
    }

    maxPayloadLength(): Promise<number> {
        return this.#sender.maxPayloadLength();
    }

    async send(data: Uint8Array): Promise<Result<void, NeighborSendError | undefined>> {
        const maxPayloadLength = await this.#sender.maxPayloadLength();
        if (data.length > maxPayloadLength) {
            throw new Error(`Payload too large: ${data.length}/${maxPayloadLength}`);
        }

        return this.#sender.send(
            new TunnelFrame({
                sourcePortId: this.#localPortId,
                destinationPortId: this.#destinationPortId,
                data,
            }),
        );
    }

    receiver(): Receiver<ReceivedTunnelFrame> {
        return this.#receiver;
    }

    close(): void {
        this.#receiver.close();
    }

    [Symbol.dispose](): void {
        this.close();
    }
}
