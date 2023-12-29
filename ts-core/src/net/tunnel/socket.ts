import { TunnelPortId } from "./port";
import { Destination } from "../node";
import { ReceivedTunnelFrame, TunnelFrame } from "./frame";
import { SingleListenerEventBroker } from "@core/event";
import { Result } from "oxide.ts";
import { NeighborSendError } from "../neighbor";

type onReceive = (info: ReceivedTunnelFrame) => void;

type SendFrame = (destination: Destination, frame: TunnelFrame) => Promise<Result<void, NeighborSendError | undefined>>;
export class TunnelSocket {
    #localPortId: TunnelPortId;
    #sendFrame: SendFrame;
    #onReceive = new SingleListenerEventBroker<ReceivedTunnelFrame>();
    #onClose: () => void;

    private constructor(args: {
        localPortId: TunnelPortId;
        sendFrame: SendFrame;
        onReceive: SingleListenerEventBroker<ReceivedTunnelFrame>;
        onClose: () => void;
    }) {
        this.#localPortId = args.localPortId;
        this.#sendFrame = args.sendFrame;
        this.#onReceive = args.onReceive;
        this.#onClose = args.onClose;
    }

    static create(args: {
        localPortId: TunnelPortId;
        sendFrame: SendFrame;
        onClose: () => void;
    }): [TunnelSocket, onReceive] {
        const onReceive = new SingleListenerEventBroker<ReceivedTunnelFrame>();
        const socket = new TunnelSocket({
            localPortId: args.localPortId,
            sendFrame: args.sendFrame,
            onReceive,
            onClose: args.onClose,
        });
        const listener = onReceive.emit.bind(onReceive);
        return [socket, listener];
    }

    get localPortId(): TunnelPortId {
        return this.#localPortId;
    }

    send(args: {
        destinationPortId: TunnelPortId;
        destination: Destination;
        data: Uint8Array;
    }): Promise<Result<void, NeighborSendError | undefined>> {
        return this.#sendFrame(
            args.destination,
            new TunnelFrame({
                sourcePortId: this.#localPortId,
                destinationPortId: args.destinationPortId,
                data: args.data,
            }),
        );
    }

    onReceive(callback: (info: ReceivedTunnelFrame) => void): void {
        this.#onReceive.listen(callback);
    }

    close(): void {
        this.#onClose();
    }
}
