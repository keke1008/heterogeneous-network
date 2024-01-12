import { Address, AddressType, Frame, LinkSendError, LinkService, LinkSocket, Protocol } from "@core/net/link";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Cost, NodeId } from "@core/net/node";
import { NeighborNode, NeighborTable } from "../table";
import { NeighborControlFlags, NeighborControlFrame } from "./frame";
import { Result } from "oxide.ts";
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
        this.#localNodeService.getInfo().then((info) => {
            this.#neighbors.initializeLocalNode(info);
        });

        this.#neighbors.onNeighborUpdated(({ neighbor, edgeCost }) => {
            this.#notificationService.notify({
                type: "NeighborUpdated",
                neighbor: neighbor.nodeId,
                linkCost: edgeCost,
            });
        });

        this.#neighbors.onHelloInterval((neighbor) => {
            if (neighbor.addresses.length !== 0) {
                this.#sendHello(neighbor.addresses[0], neighbor.edgeCost, NeighborControlFlags.KeepAlive);
            }
        });
    }

    onNeighborAdded(listener: (neighbor: Readonly<NeighborNode>) => void): CancelListening {
        return this.#neighbors.onNeighborAdded(listener);
    }

    onNeighborRemoved(listener: (neighborId: NodeId) => void): CancelListening {
        return this.#neighbors.onNeighborRemoved(listener);
    }

    async #onFrameReceived(linkFrame: Frame): Promise<void> {
        const resultNeighborFrame = BufferReader.deserialize(
            NeighborControlFrame.serdeable.deserializer(),
            linkFrame.payload,
        );
        if (resultNeighborFrame.isErr()) {
            console.warn(`NeighborService: failed to deserialize frame with error: ${resultNeighborFrame.unwrapErr()}`);
            return;
        }

        const frame = resultNeighborFrame.unwrap();
        const delayCost = frame.linkCost.add(await this.#localNodeService.getCost()).intoDuration();
        await sleep(delayCost);

        this.#neighbors.addNeighbor(frame.source, frame.linkCost, linkFrame.remote);
        if (!frame.shouldReplyImmediately()) {
            return;
        }

        const result = await this.#sendHello(linkFrame.remote, frame.linkCost, NeighborControlFlags.KeepAlive);
        if (result.isOk()) {
            this.#neighbors.delayHelloInterval(frame.source.nodeId);
        } else {
            console.warn(`NeighborService: failed to send reply frame to ${linkFrame.remote}`, result.unwrapErr());
        }
    }

    async #sendHello(
        destination: Address,
        linkCost: Cost,
        flags: NeighborControlFlags,
    ): Promise<Result<void, LinkSendError>> {
        const { source, cost: sourceCost } = await this.#localNodeService.getInfo();
        const frame = new NeighborControlFrame({ flags, source, sourceCost, linkCost });
        const buffer = BufferWriter.serialize(NeighborControlFrame.serdeable.serializer(frame)).unwrap();
        return this.#socket.send(destination, buffer);
    }

    async sendHello(destination: Address, linkCost: Cost): Promise<Result<void, LinkSendError>> {
        return this.#sendHello(destination, linkCost, NeighborControlFlags.Empty);
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

    onFrameSent(destination: NodeId | AddressType) {
        this.#neighbors.delayHelloInterval(destination);
    }

    onFrameReceived(nodeId: NodeId) {
        this.#neighbors.delayExpiration(nodeId);
    }
}
