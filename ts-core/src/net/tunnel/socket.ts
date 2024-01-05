import { TunnelPortId } from "./port";
import { Destination } from "../node";
import { ReceivedTunnelFrame, TunnelFrame } from "./frame";
import { Result } from "oxide.ts";
import { NeighborSendError } from "../neighbor";
import { Receiver } from "@core/channel";

type SendFrame = (destination: Destination, frame: TunnelFrame) => Promise<Result<void, NeighborSendError | undefined>>;
export class TunnelSocket {
    #localPortId: TunnelPortId;
    #destination: Destination;
    #destinationPortId: TunnelPortId;

    #sendFrame: SendFrame;
    #receiver: Receiver<ReceivedTunnelFrame>;

    constructor(args: {
        localPortId: TunnelPortId;
        destination: Destination;
        destinationPortId: TunnelPortId;
        sendFrame: SendFrame;
        receiver: Receiver<ReceivedTunnelFrame>;
    }) {
        this.#localPortId = args.localPortId;
        this.#destination = args.destination;
        this.#destinationPortId = args.destinationPortId;
        this.#sendFrame = args.sendFrame;
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

    send(data: Uint8Array): Promise<Result<void, NeighborSendError | undefined>> {
        return this.#sendFrame(
            this.#destination,
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
