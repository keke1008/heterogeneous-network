import { Address, LinkService, Protocol } from "../link";
import { Cost, NodeId, ReactiveService, RoutingFrame, RoutingSocket } from "../routing";
import { Procedure, RpcRequest, RpcStatus, serializeFrame } from "./frame";
import { RpcServer, ProcedureHandler } from "./procedures";
import { RpcResult } from "./request";

export class RpcService {
    #handler: ProcedureHandler;
    #socket: RoutingSocket;

    constructor(args: { linkService: LinkService; reactiveService: ReactiveService }) {
        this.#handler = new ProcedureHandler({ reactiveService: args.reactiveService });

        const linkSocket = args.linkService.open(Protocol.Rpc);
        this.#socket = new RoutingSocket(linkSocket, args.reactiveService);
        this.#socket.onReceive((frame) => this.#handleReceive(frame));
    }

    async #handleReceive(frame: RoutingFrame): Promise<void> {
        const response = await this.#handler.handleFrame(frame);
        if (response !== undefined) {
            this.#socket.send(frame.senderId, serializeFrame(response));
        }
    }

    async #sendRequest(destinationId: NodeId, request: RpcRequest): Promise<RpcResult<never> | undefined> {
        const sendResult = await this.#socket.send(destinationId, serializeFrame(request));
        if (sendResult.result === "failure") {
            return { status: RpcStatus.Unreachable };
        }
    }

    addServer(procedure: Procedure, server: RpcServer): void {
        this.#handler.addServer(procedure, server);
    }

    async requestRoutingNeighborSendHello(
        destinationId: NodeId,
        targetAddress: Address,
        edgeCost: Cost,
    ): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.RoutingNeighborSendHello);
        const [request, result] = handler.createRequest(destinationId, targetAddress, edgeCost);
        return (await this.#sendRequest(destinationId, request)) ?? result;
    }

    async requestRoutingNeighborSendGoodbye(destinationId: NodeId, targetNode: NodeId): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.RoutingNeighborSendGoodbye);
        const [request, result] = handler.createRequest(destinationId, targetNode);
        return (await this.#sendRequest(destinationId, request)) ?? result;
    }

    async requestRoutingNeighborGetNeighborList(destinationId: NodeId): Promise<RpcResult<NodeId[]>> {
        const handler = this.#handler.getClient(Procedure.RoutingNeighborGetNeighborList);
        const [request, result] = handler.createRequest(destinationId);
        return (await this.#sendRequest(destinationId, request)) ?? result;
    }
}
