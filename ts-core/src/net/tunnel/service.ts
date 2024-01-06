import { ObjectMap, UniqueKey } from "@core/object";
import { Err, Ok, Result } from "oxide.ts";
import { BufferWriter } from "../buffer";
import { NeighborService } from "../neighbor";
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
import { NotificationService } from "../notification";

class PortIdentifier implements UniqueKey {
    destination: Destination;
    destinationPortId: TunnelPortId;

    constructor(args: { destination: Destination; destinationPortId: TunnelPortId }) {
        this.destination = args.destination;
        this.destinationPortId = args.destinationPortId;
    }

    uniqueKey(): Keyable {
        return `(${this.destination.uniqueKey()}):(${this.destinationPortId.uniqueKey()})`;
    }
}

class Port {
    #frameSender = new ObjectMap<PortIdentifier, Sender<ReceivedTunnelFrame>>();
    #listener?: (socket: TunnelSocket) => void;
    #onEmpty: () => void;

    constructor(args: { onEmpty: () => void }) {
        this.#onEmpty = args.onEmpty;
    }

    #onClose(args: { identifier: PortIdentifier; sender: Sender<ReceivedTunnelFrame> }): void {
        if (this.#frameSender.get(args.identifier) === args.sender) {
            this.#frameSender.delete(args.identifier);
        }

        if (this.#frameSender.size === 0 && this.#listener === undefined) {
            this.#onEmpty();
        }
    }

    #open(args: {
        routingSocket: RoutingSocket;
        localPortId: TunnelPortId;
        destination: Destination;
        destinationPortId: TunnelPortId;
        sender: Sender<ReceivedTunnelFrame>;
    }): Result<TunnelSocket, "already opened"> {
        const identifier = new PortIdentifier(args);
        if (this.#frameSender.has(identifier)) {
            return Err("already opened");
        }

        const socket = new TunnelSocket({
            localPortId: args.localPortId,
            destination: args.destination,
            destinationPortId: args.destinationPortId,
            sendFrame: (frame) => {
                const buffer = BufferWriter.serialize(TunnelFrame.serdeable.serializer(frame)).unwrap();
                return args.routingSocket.send(args.destination, buffer);
            },
            receiver: args.sender.receiver(),
        });

        this.#frameSender.set(identifier, args.sender);
        args.sender.onClose(() => this.#onClose({ identifier, sender: args.sender }));
        return Ok(socket);
    }

    open(args: {
        routingSocket: RoutingSocket;
        localPortId: TunnelPortId;
        destination: Destination;
        destinationPortId: TunnelPortId;
    }): Result<TunnelSocket, "already opened"> {
        return this.#open({
            routingSocket: args.routingSocket,
            localPortId: args.localPortId,
            destination: args.destination,
            destinationPortId: args.destinationPortId,
            sender: new Sender<ReceivedTunnelFrame>(),
        });
    }

    listen(callback: (socket: TunnelSocket) => void): Result<() => void, "already opened"> {
        if (this.#listener !== undefined) {
            return Err("already opened");
        }

        const callbackWrapper = (socket: TunnelSocket) => callback(socket);
        this.#listener = callbackWrapper;

        const cancel = () => {
            if (this.#listener === callbackWrapper) {
                this.#listener = undefined;
            }
        };
        return Ok(cancel);
    }

    handleReceivedFrame(routingSocket: RoutingSocket, frame: ReceivedTunnelFrame): void {
        const identifier = new PortIdentifier({
            destination: frame.source.intoDestination(),
            destinationPortId: frame.sourcePortId,
        });
        const sender = this.#frameSender.get(identifier);
        if (sender) {
            sender.send(frame);
            return;
        }

        if (this.#listener !== undefined) {
            const sender = new Sender<ReceivedTunnelFrame>();
            const socket = this.#open({
                routingSocket,
                localPortId: frame.destinationPortId,
                destination: frame.source.intoDestination(),
                destinationPortId: frame.sourcePortId,
                sender,
            }).unwrap();

            sender.send(frame);
            this.#listener(socket);
        }
    }
}

export class TunnelService {
    #notificationService: NotificationService;
    #socket: RoutingSocket;
    #ports = new ObjectMap<TunnelPortId, Port>();

    constructor(args: {
        linkService: LinkService;
        notificationService: NotificationService;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        routingService: RoutingService;
    }) {
        this.#notificationService = args.notificationService;
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

            this.#notificationService.notify({ type: "FrameReceived" });
            const frame = result.unwrap();
            const port = this.#ports.get(frame.destinationPortId);
            port?.handleReceivedFrame(this.#socket, frame);
        });
    }

    open(args: {
        localPortId: TunnelPortId;
        destination: Destination;
        destinationPortId: TunnelPortId;
    }): Result<TunnelSocket, "already opened"> {
        if (this.#ports.has(args.localPortId)) {
            return Err("already opened");
        }

        const port = new Port({ onEmpty: () => this.#ports.delete(args.localPortId) });
        this.#ports.set(args.localPortId, port);
        return port.open({
            routingSocket: this.#socket,
            localPortId: args.localPortId,
            destination: args.destination,
            destinationPortId: args.destinationPortId,
        });
    }

    openDynamicPort(args: {
        destination: Destination;
        destinationPortId: TunnelPortId;
    }): Result<TunnelSocket, "already opened"> {
        let localPortId = TunnelPortId.generateRandomDynamicPort();
        while (this.#ports.has(localPortId)) {
            localPortId = TunnelPortId.generateRandomDynamicPort();
        }

        return this.open({
            localPortId,
            destination: args.destination,
            destinationPortId: args.destinationPortId,
        });
    }

    listen(localPortId: TunnelPortId, callback: (socket: TunnelSocket) => void): Result<() => void, "already opened"> {
        const port = this.#ports.get(localPortId);
        if (port) {
            return port.listen(callback);
        } else {
            const port = new Port({ onEmpty: () => this.#ports.delete(localPortId) });
            this.#ports.set(localPortId, port);
            return port.listen(callback);
        }
    }
}
