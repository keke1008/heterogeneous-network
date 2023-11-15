import { Address, Frame, LinkSendError, LinkService, LinkSocket, Protocol } from "@core/net/link";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Cost, NodeId } from "../node";
import { NeighborNode, NeighborTable } from "./table";
import { FrameType, GoodbyeFrame, HelloFrame, deserializeFrame } from "./frame";
import { Ok, Result } from "oxide.ts";

export type NeighborEvent =
    | { type: "neighbor added"; peerId: NodeId; edge_cost: Cost }
    | { type: "neighbor removed"; peerId: NodeId };

export class NeighborService {
    #neighbors: NeighborTable = new NeighborTable();
    #socket: LinkSocket;
    #localId: NodeId;
    #onEvent: ((event: NeighborEvent) => void) | undefined;

    constructor(args: { linkService: LinkService; localId: NodeId }) {
        this.#socket = args.linkService.open(Protocol.RoutingNeighbor);
        this.#localId = args.localId;

        this.#socket.onReceive((frame) => this.#onFrameReceived(frame));
    }

    #onFrameReceived(frame: Frame): void {
        const resultNeighborFrame = deserializeFrame(frame.reader);
        if (resultNeighborFrame.isErr()) {
            console.warn(`NeighborService: failed to deserialize frame with error: ${resultNeighborFrame.unwrapErr()}`);
            return;
        }

        const neighborFrame = resultNeighborFrame.unwrap();
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

    #sendFrame(frame: HelloFrame | GoodbyeFrame, destination: Address): Result<void, LinkSendError> {
        const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
        frame.serialize(writer);
        return this.#socket.send(destination, new BufferReader(writer.unwrapBuffer()));
    }

    sendHello(destination: Address, edgeCost: Cost): Result<void, LinkSendError> {
        const frame = new HelloFrame({ type: FrameType.Hello, senderId: this.#localId, edgeCost });
        return this.#sendFrame(frame, destination);
    }

    #replyHelloAck(frame: HelloFrame, destination: Address): Result<void, LinkSendError> {
        const reply = new HelloFrame({ type: FrameType.HelloAck, senderId: this.#localId, edgeCost: frame.edgeCost });
        return this.#sendFrame(reply, destination);
    }

    sendGoodbye(destination: NodeId): Result<void, LinkSendError> {
        const frame = new GoodbyeFrame({ senderId: this.#localId });
        const addrs = this.#neighbors.getNeighbor(destination)?.addresses;
        this.#neighbors.removeNeighbor(destination);
        if (addrs !== undefined && addrs.length > 0) {
            return this.#sendFrame(frame, addrs[0]);
        } else {
            return Ok(undefined);
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
