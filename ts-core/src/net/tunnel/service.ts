import { ObjectMap, UniqueKey } from "@core/object";
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
import { Sender } from "@core/channel";
import { Keyable } from "@core/types";

class SocketIdentifier implements UniqueKey {
    localPortId: TunnelPortId;
    destination: Destination;
    destinationPortId: TunnelPortId;

    constructor(args: { localPortId: TunnelPortId; destination: Destination; destinationPortId: TunnelPortId }) {
        this.localPortId = args.localPortId;
        this.destination = args.destination;
        this.destinationPortId = args.destinationPortId;
    }

    static fromFrame(frame: ReceivedTunnelFrame): SocketIdentifier {
        return new SocketIdentifier({
            localPortId: frame.destinationPortId,
            destination: frame.destination,
            destinationPortId: frame.sourcePortId,
        });
    }

    uniqueKey(): Keyable {
        return `(${this.localPortId.uniqueKey()}):(${this.destination.uniqueKey()}):(${this.destinationPortId.uniqueKey()})`;
    }
}

export class TunnelService {
    #socket: RoutingSocket;
    #frameReceive = new ObjectMap<SocketIdentifier, Sender<ReceivedTunnelFrame>>();
    #listener = new ObjectMap<TunnelPortId, (socket: TunnelSocket) => void>();

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
            const sender = this.#frameReceive.get(SocketIdentifier.fromFrame(frame));
            if (sender) {
                sender.send(frame);
                return;
            }

            const listener = this.#listener.get(frame.destinationPortId);
            if (listener) {
                const socket = this.open({
                    localPortId: frame.destinationPortId,
                    destination: frame.destination,
                    destinationPortId: frame.sourcePortId,
                });
                if (socket.isErr()) {
                    console.warn(`failed to open socket: ${socket.unwrapErr()}`);
                    return;
                }

                const sender = this.#frameReceive.get(SocketIdentifier.fromFrame(frame));
                if (!sender) {
                    throw new Error("Failed to get sender");
                }
                sender.send(frame);
                listener(socket.unwrap());
            }
        });
    }

    #sendFrame(args: {
        destination: Destination;
        frame: TunnelFrame;
    }): Promise<Result<void, NeighborSendError | undefined>> {
        const buffer = BufferWriter.serialize(TunnelFrame.serdeable.serializer(args.frame)).expect(
            "Failed to serialize frame",
        );
        return this.#socket.send(args.destination, buffer);
    }

    #onSocketClose(args: { identifier: SocketIdentifier; sender: Sender<ReceivedTunnelFrame> }): void {
        if (this.#frameReceive.get(args.identifier) === args.sender) {
            this.#frameReceive.delete(args.identifier);
        }
    }

    open(args: {
        localPortId: TunnelPortId;
        destination: Destination;
        destinationPortId: TunnelPortId;
    }): Result<TunnelSocket, "already opened"> {
        const identifier = new SocketIdentifier(args);
        if (this.#frameReceive.has(identifier)) {
            return Err("already opened");
        }

        const sender = new Sender<ReceivedTunnelFrame>();
        const socket = new TunnelSocket({
            localPortId: args.localPortId,
            destination: args.destination,
            destinationPortId: args.destinationPortId,
            sendFrame: (destination, frame) => this.#sendFrame({ destination, frame }),
            receiver: sender.receiver(),
        });

        this.#frameReceive.set(identifier, sender);
        sender.onClose(() => this.#onSocketClose({ identifier, sender }));
        return Ok(socket);
    }

    listen(localPortId: TunnelPortId, callback: (socket: TunnelSocket) => void): Result<() => void, "already opened"> {
        if (this.#listener.has(localPortId)) {
            return Err("already opened");
        }

        const callbackWrapper = (socket: TunnelSocket) => callback(socket);
        this.#listener.set(localPortId, callbackWrapper);

        const cancel = () => {
            if (this.#listener.get(localPortId) === callbackWrapper) {
                this.#listener.delete(localPortId);
            }
        };
        return Ok(cancel);
    }
}
