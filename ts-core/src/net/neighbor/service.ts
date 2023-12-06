import { Address, Frame, LinkSendError, LinkService, LinkSocket, Protocol } from "@core/net/link";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Cost, NodeId, LocalNodeInfo } from "@core/net/node";
import { NeighborNode, NeighborTable } from "./table";
import { FrameType, GoodbyeFrame, HelloFrame, deserializeFrame } from "./frame";
import { Ok, Result } from "oxide.ts";
import { NotificationService } from "@core/net/notification";

export class NeighborService {
    #notificationService: NotificationService;

    #neighbors: NeighborTable = new NeighborTable();
    #socket: LinkSocket;
    #localNodeInfo: LocalNodeInfo;

    constructor(args: {
        notificationService: NotificationService;
        linkService: LinkService;
        localNodeInfo: LocalNodeInfo;
    }) {
        this.#notificationService = args.notificationService;
        this.#socket = args.linkService.open(Protocol.RoutingNeighbor);
        this.#localNodeInfo = args.localNodeInfo;
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
            this.#notificationService.notify({ type: "NeighborRemoved", nodeId: neighborFrame.senderId });
            return;
        }

        this.#neighbors.addNeighbor(neighborFrame.senderId, neighborFrame.linkCost, frame.remote);
        if (neighborFrame.type === FrameType.Hello) {
            this.#replyHelloAck(neighborFrame, frame.remote);
        }
        this.#notificationService.notify({
            type: "NeighborUpdated",
            neighborId: neighborFrame.senderId,
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
        return this.#sendFrame(
            new HelloFrame({
                type: FrameType.Hello,
                senderId: await this.#localNodeInfo.getId(),
                nodeCost: await this.#localNodeInfo.getCost(),
                linkCost,
            }),
            destination,
        );
    }

    async #replyHelloAck(frame: HelloFrame, destination: Address): Promise<Result<void, LinkSendError>> {
        return this.#sendFrame(
            new HelloFrame({
                type: FrameType.HelloAck,
                senderId: await this.#localNodeInfo.getId(),
                nodeCost: await this.#localNodeInfo.getCost(),
                linkCost: frame.linkCost,
            }),
            destination,
        );
    }

    async sendGoodbye(destination: NodeId): Promise<Result<void, LinkSendError>> {
        const frame = new GoodbyeFrame({ senderId: await this.#localNodeInfo.getId() });
        const addrs = this.#neighbors.getNeighbor(destination)?.addresses;
        this.#neighbors.removeNeighbor(destination);
        if (addrs !== undefined && addrs.length > 0) {
            return this.#sendFrame(frame, addrs[0]);
        } else {
            return Ok(undefined);
        }
    }

    async resolveAddress(id: NodeId): Promise<Address[]> {
        const convertedId = await this.#localNodeInfo.convertLocalNodeIdToLoopback(id);
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
