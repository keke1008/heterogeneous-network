import { Address, Frame, LinkSendError, LinkService, LinkSocket, Protocol } from "@core/net/link";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Cost, NodeId } from "@core/net/node";
import { NeighborNode, NeighborTable } from "../table";
import { FrameType, GoodbyeFrame, HelloFrame, NeighborFrame } from "./frame";
import { Ok, Result } from "oxide.ts";
import { NotificationService } from "@core/net/notification";
import { CancelListening } from "@core/event";
import { LocalNodeService } from "@core/net/local";
import { sleep } from "@core/async";

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
        this.#localNodeService.getSource().then((source) => {
            this.#neighbors.addNeighbor(source, new Cost(0), Address.loopback());
        });
    }

    onNeighborAdded(listener: (neighbor: Readonly<NeighborNode>) => void): CancelListening {
        return this.#neighbors.onNeighborAdded(listener);
    }

    onNeighborRemoved(listener: (neighborId: NodeId) => void): CancelListening {
        return this.#neighbors.onNeighborRemoved(listener);
    }

    async #onFrameReceived(frame: Frame): Promise<void> {
        const resultNeighborFrame = BufferReader.deserialize(NeighborFrame.serdeable.deserializer(), frame.payload);
        if (resultNeighborFrame.isErr()) {
            console.warn(`NeighborService: failed to deserialize frame with error: ${resultNeighborFrame.unwrapErr()}`);
            return;
        }

        const neighborFrame = resultNeighborFrame.unwrap();
        const linkCost = neighborFrame.type === FrameType.Goodbye ? new Cost(0) : neighborFrame.linkCost;
        const delayCost = linkCost.add(await this.#localNodeService.getCost()).intoDuration();
        await sleep(delayCost);

        if (neighborFrame.type === FrameType.Goodbye) {
            this.#neighbors.removeNeighbor(neighborFrame.senderId);
            this.#notificationService.notify({ type: "NeighborRemoved", nodeId: neighborFrame.senderId });
            return;
        }
        if (neighborFrame.type === FrameType.Hello) {
            await this.#replyHelloAck(neighborFrame, frame.remote);
        }

        this.#neighbors.addNeighbor(neighborFrame.source, neighborFrame.linkCost, frame.remote);
        this.#notificationService.notify({
            type: "NeighborUpdated",
            neighbor: neighborFrame.source,
            neighborCost: neighborFrame.nodeCost,
            linkCost: neighborFrame.linkCost,
        });
    }

    #sendFrame(frame: HelloFrame | GoodbyeFrame, destination: Address): Result<void, LinkSendError> {
        const buffer = BufferWriter.serialize(NeighborFrame.serdeable.serializer(frame)).expect(
            "Failed to serialize neighbor frame",
        );
        return this.#socket.send(destination, buffer);
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
