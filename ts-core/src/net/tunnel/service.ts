import { SingleListenerEventBroker } from "@core/event";
import { ObjectMap } from "@core/object";
import { Err, Ok, Result } from "oxide.ts";
import { BufferWriter } from "../buffer";
import { NeighborSendError, NeighborService } from "../neighbor";
import { Destination } from "../node";
import { RoutingService, RoutingSocket } from "../routing";
import { ReceivedTunnelFrame, TunnelFrame } from "./frame";
import { TunnelPortId } from "./port";
import { LinkService, Protocol } from "../link";
import { LocalNodeService } from "../local";
import { TunnelSocket } from "./socket";
import { MAX_FRAME_ID_CACHE_SIZE } from "./constants";

type TunnelFrameListener = SingleListenerEventBroker<ReceivedTunnelFrame>;

export class TunnelService {
    #socket: RoutingSocket;
    #frameReceive = new ObjectMap<TunnelPortId, TunnelFrameListener, string>((id) => id.toUniqueString());

    constructor(args: {
        linkService: LinkService;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        routingService: RoutingService;
    }) {
        this.#socket = new RoutingSocket({
            linkSocket: args.linkService.open(Protocol.Tunnel),
            localNodeService: args.localNodeService,
            neighborService: args.neighborService,
            routingService: args.routingService,
            maxFrameIdCacheSize: MAX_FRAME_ID_CACHE_SIZE,
        });
        this.#socket.onReceive((routingFrame) => {
            const result = ReceivedTunnelFrame.deserializeFromRoutingFrame(routingFrame);
            if (result.isErr()) {
                console.warn(`dropping invalid frame: ${result.unwrapErr()}`);
                return;
            }

            const frame = result.unwrap();
            const receiver = this.#frameReceive.get(frame.destinationPortId);
            if (receiver) {
                receiver.emit(frame);
            } else {
                console.warn(`dropping frame for unknown port ${frame.destinationPortId.toUniqueString()}`);
            }
        });
    }

    #sendFrame(args: {
        destination: Destination;
        frame: TunnelFrame;
    }): Promise<Result<void, NeighborSendError | undefined>> {
        const buffer = BufferWriter.serialize(TunnelFrame.serdeable.serializer(args.frame));
        return this.#socket.send(args.destination, buffer);
    }

    #onSocketClose(args: { localPortId: TunnelPortId; listener: TunnelFrameListener }): void {
        if (this.#frameReceive.get(args.localPortId) === args.listener) {
            this.#frameReceive.delete(args.localPortId);
        }
    }

    open(args: { localPortId: TunnelPortId }): Result<TunnelSocket, "already opened"> {
        if (this.#frameReceive.has(args.localPortId)) {
            return Err("already opened");
        }

        const listener = new SingleListenerEventBroker<ReceivedTunnelFrame>();
        const [socket, onReceive] = TunnelSocket.create({
            localPortId: args.localPortId,
            sendFrame: (destination, frame) => this.#sendFrame({ destination, frame }),
            onClose: () => this.#onSocketClose({ localPortId: args.localPortId, listener }),
        });

        listener.listen(onReceive);
        this.#frameReceive.set(args.localPortId, listener);
        return Ok(socket);
    }
}
