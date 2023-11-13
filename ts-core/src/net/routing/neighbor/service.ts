import { Address, Frame, LinkService, LinkSocket, Protocol } from "@core/net/link";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Cost, NodeId } from "../node";
import { NeighborNode, NeighborTable } from "./table";
import { FrameType, GoodbyeFrame, HelloFrame, deserializeFrame } from "./frame";

export type NeighborEvent =
    | { type: "neighbor added"; peerId: NodeId; edge_cost: Cost }
    | { type: "neighbor removed"; peerId: NodeId };

export class NeighborService {
    #neighbors: NeighborTable = new NeighborTable();
    #socket: LinkSocket;
    #selfId: NodeId;
    #onEvent: ((event: NeighborEvent) => void) | undefined;

    constructor(linkService: LinkService, selfId: NodeId) {
        this.#socket = linkService.open(Protocol.RoutingNeighbor);
        this.#selfId = selfId;

        this.#socket.onReceive((frame) => this.#onFrameReceived(frame));
    }

    #onFrameReceived(frame: Frame): void {
        const neighborFrame = deserializeFrame(frame.reader);
        if (neighborFrame.type === FrameType.Goodbye) {
            this.#neighbors.removeNeighbor(neighborFrame.senderId);
            this.#onEvent?.({ type: "neighbor removed", peerId: neighborFrame.senderId });
            return;
        }

        this.#neighbors.addNeighbor(neighborFrame.senderId, neighborFrame.edgeCost, frame.remote);
        if (neighborFrame.type === FrameType.Hello) {
            this.#replyHelloAck(neighborFrame, frame.remote);
        }
        this.#onEvent?.({ type: "neighbor added", peerId: neighborFrame.senderId, edge_cost: neighborFrame.edgeCost });
    }

    onEvent(callback: (event: NeighborEvent) => void): void {
        if (this.#onEvent !== undefined) {
            throw new Error("NeighborService.onEvent: callback already set");
        }

        this.#onEvent = callback;
    }

    #sendFrame(frame: HelloFrame | GoodbyeFrame, destination: Address): void {
        const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
        frame.serialize(writer);
        this.#socket.send(destination, new BufferReader(writer.unwrapBuffer()));
    }

    sendHello(destination: Address, edgeCost: Cost): void {
        const frame = new HelloFrame({ type: FrameType.Hello, senderId: this.#selfId, edgeCost });
        this.#sendFrame(frame, destination);
    }

    #replyHelloAck(frame: HelloFrame, destination: Address): void {
        const reply = new HelloFrame({ type: FrameType.HelloAck, senderId: this.#selfId, edgeCost: frame.edgeCost });
        this.#sendFrame(reply, destination);
    }

    sendGoodbye(destination: NodeId): void {
        const frame = new GoodbyeFrame({ senderId: this.#selfId });
        const addrs = this.#neighbors.getNeighbor(destination)?.addresses;
        this.#neighbors.removeNeighbor(destination);
        if (addrs !== undefined && addrs.length > 0) {
            this.#sendFrame(frame, addrs[0]);
        }
    }

    resolveAddress(id: NodeId): Address[] {
        return this.#neighbors.getAddresses(id);
    }

    getNeighbor(id: NodeId): NeighborNode | undefined {
        return this.#neighbors.getNeighbor(id);
    }

    getNeighbors(): NeighborNode[] {
        return this.#neighbors.getNeighbors();
    }
}
