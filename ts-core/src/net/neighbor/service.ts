import { Address, Frame, LinkSendError, LinkService, LinkSocket, Protocol } from "@core/net/link";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Cost, NodeId, LocalNodeService } from "@core/net/node";
import { NeighborNode, NeighborTable } from "./table";
import { FrameType, GoodbyeFrame, HelloFrame, NeighborFrame } from "./frame";
import { Ok, Result } from "oxide.ts";
import { NotificationService } from "@core/net/notification";
import { CancelListening } from "@core/event";

export class NeighborService {
    #notificationService: NotificationService;
    #localNodeService: LocalNodeService;

    #neighbors: NeighborTable = new NeighborTable();
    #socket: LinkSocket;

    constructor(args: {
        notificationService: NotificationService;
        linkService: LinkService;
        localNodeService: LocalNodeService;
    }) {
        this.#notificationService = args.notificationService;
        this.#localNodeService = args.localNodeService;
        this.#socket = args.linkService.open(Protocol.RoutingNeighbor);
        this.#socket.onReceive((frame) => this.#onFrameReceived(frame));
    }

    onNeighborAdded(listener: (neighbor: Readonly<NeighborNode>) => void): CancelListening {
        return this.#neighbors.onNeighborAdded(listener);
    }

    onNeighborRemoved(listener: (neighborId: NodeId) => void): CancelListening {
        return this.#neighbors.onNeighborRemoved(listener);
    }

    async #onFrameReceived(frame: Frame): Promise<void> {
        const resultNeighborFrame = NeighborFrame.deserialize(frame.reader);
        if (resultNeighborFrame.isErr()) {
            console.warn(`NeighborService: failed to deserialize frame with error: ${resultNeighborFrame.unwrapErr()}`);
            return;
        }

        const neighborFrame = resultNeighborFrame.unwrap();
        if (neighborFrame.type === FrameType.Goodbye) {
            this.#neighbors.removeNeighbor(neighborFrame.senderId);
            this.#notificationService.notify({ type: "NeighborRemoved", nodeId: neighborFrame.senderId });
            return;
        }

        this.#neighbors.addNeighbor(neighborFrame.source.nodeId, neighborFrame.linkCost, frame.remote);
        if (neighborFrame.type === FrameType.Hello) {
            await this.#replyHelloAck(neighborFrame, frame.remote);
        }
        this.#notificationService.notify({
            type: "NeighborUpdated",
            neighborId: neighborFrame.source.nodeId,
            neighborCost: neighborFrame.nodeCost,
            linkCost: neighborFrame.linkCost,
        });
    }

    #sendFrame(frame: HelloFrame | GoodbyeFrame, destination: Address): Result<void, LinkSendError> {
        const writer = new BufferWriter(new Uint8Array(frame.serializedLength()));
        frame.serialize(writer);
        return this.#socket.send(destination, new BufferReader(writer.unwrapBuffer()));
    }

    async sendHello(destination: Address, linkCost: Cost): Promise<Result<void, LinkSendError>> {
        const { source, cost: nodeCost } = await this.#localNodeService.getInfo();
        return this.#sendFrame(new HelloFrame({ type: FrameType.Hello, source, nodeCost, linkCost }), destination);
    }

    async #replyHelloAck(frame: HelloFrame, destination: Address): Promise<Result<void, LinkSendError>> {
        const { source, cost: nodeCost } = await this.#localNodeService.getInfo();
        return this.#sendFrame(
            new HelloFrame({ type: FrameType.HelloAck, source, nodeCost, linkCost: frame.linkCost }),
            destination,
        );
    }

    async sendGoodbye(destination: NodeId): Promise<Result<void, LinkSendError>> {
        const frame = new GoodbyeFrame({ senderId: await this.#localNodeService.getId() });
        const addrs = this.#neighbors.getNeighbor(destination)?.addresses;
        this.#neighbors.removeNeighbor(destination);
        if (addrs !== undefined && addrs.length > 0) {
            return this.#sendFrame(frame, addrs[0]);
        } else {
            return Ok(undefined);
        }
    }

    async resolveAddress(id: NodeId): Promise<Address[]> {
        const convertedId = await this.#localNodeService.convertLocalNodeIdToLoopback(id);
        return this.#neighbors.getAddresses(convertedId);
    }

    hasNeighbor(id: NodeId): boolean {
        return this.#neighbors.getNeighbor(id) !== undefined;
    }

    getNeighbor(id: NodeId): NeighborNode | undefined {
        return this.#neighbors.getNeighbor(id);
    }

    getNeighbors(): NeighborNode[] {
        return this.#neighbors.getNeighbors();
    }
}
