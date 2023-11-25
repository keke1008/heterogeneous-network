import { Address, LinkService, Protocol } from "../link";
import { Cost, NodeId, ReactiveService, RoutingFrame, RoutingSocket } from "../routing";
import { Procedure, RpcRequest, RpcStatus, serializeFrame } from "./frame";
import { RpcServer, ProcedureHandler, BlinkOperation, MediaInfo } from "./procedures";
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
        const response = await this.#handler.handleReceivedFrame(frame);
        if (response !== undefined) {
            this.#socket.send(frame.senderId, serializeFrame(response));
        }
    }

    #sendRequest(destinationId: NodeId, request: RpcRequest): RpcResult<never> | undefined {
        const sendResult = this.#socket.send(destinationId, serializeFrame(request));
        if (sendResult.isErr()) {
            return { status: RpcStatus.Unreachable };
        }
    }

    addServer(procedure: Procedure, server: RpcServer): void {
        this.#handler.addServer(procedure, server);
    }

    async requestBlink(destinationId: NodeId, operation: BlinkOperation): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.Blink);
        const [request, result] = handler.createRequest(destinationId, operation);
        return this.#sendRequest(destinationId, request) ?? result;
    }

    async requestGetMediaList(destinationId: NodeId): Promise<RpcResult<MediaInfo[]>> {
        const handler = this.#handler.getClient(Procedure.GetMediaList);
        const [request, result] = handler.createRequest(destinationId);
        return this.#sendRequest(destinationId, request) ?? result;
    }

    async requestConnectToAccessPoint(
        destinationId: NodeId,
        ssid: Uint8Array,
        password: Uint8Array,
    ): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.ConnectToAccessPoint);
        const [request, result] = handler.createRequest(destinationId, ssid, password);
        return this.#sendRequest(destinationId, request) ?? result;
    }

    async requestStartServer(destinationId: NodeId, port: number): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.StartServer);
        const [request, result] = handler.createRequest(destinationId, port);
        return this.#sendRequest(destinationId, request) ?? result;
    }

    async requestSendHello(destinationId: NodeId, targetAddress: Address, linkCost: Cost): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.SendHello);
        const [request, result] = handler.createRequest(destinationId, targetAddress, linkCost);
        return this.#sendRequest(destinationId, request) ?? result;
    }

    async requestSendGoodbye(destinationId: NodeId, targetNode: NodeId): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.SendGoodbye);
        const [request, result] = handler.createRequest(destinationId, targetNode);
        return this.#sendRequest(destinationId, request) ?? result;
    }
}
