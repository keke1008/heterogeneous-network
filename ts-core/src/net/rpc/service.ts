import { Address, LinkService, Protocol } from "../link";
import { Cost, NodeId } from "../node";
import { ReactiveService, RoutingFrame, RoutingSocket } from "../routing";
import { MAX_FRAME_ID_CACHE_SIZE } from "./constants";
import { Procedure, RpcRequest, RpcStatus, serializeFrame } from "./frame";
import { RpcServer, ProcedureHandler, BlinkOperation, MediaInfo } from "./procedures";
import { RpcResult } from "./request";

export class RpcService {
    #handler: ProcedureHandler;
    #socket: RoutingSocket;

    constructor(args: { linkService: LinkService; reactiveService: ReactiveService }) {
        this.#handler = new ProcedureHandler({ reactiveService: args.reactiveService });

        const linkSocket = args.linkService.open(Protocol.Rpc);
        this.#socket = new RoutingSocket(linkSocket, args.reactiveService, MAX_FRAME_ID_CACHE_SIZE);
        this.#socket.onReceive((frame) => this.#handleReceive(frame));
    }

    async #handleReceive(frame: RoutingFrame): Promise<void> {
        const response = await this.#handler.handleReceivedFrame(frame);
        if (response !== undefined) {
            this.#socket.send(frame.header.senderId, serializeFrame(response));
        }
    }

    async #sendRequest(destinationId: NodeId, request: RpcRequest): Promise<RpcResult<never> | undefined> {
        const sendResult = await this.#socket.send(destinationId, serializeFrame(request));
        if (sendResult.isErr()) {
            return { status: RpcStatus.Unreachable };
        }
    }

    addServer(procedure: Procedure, server: RpcServer): void {
        this.#handler.addServer(procedure, server);
    }

    async requestBlink(destinationId: NodeId, operation: BlinkOperation): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.Blink);
        const [request, result] = await handler.createRequest(destinationId, operation);
        return (await this.#sendRequest(destinationId, request)) ?? result;
    }

    async requestGetMediaList(destinationId: NodeId): Promise<RpcResult<MediaInfo[]>> {
        const handler = this.#handler.getClient(Procedure.GetMediaList);
        const [request, result] = await handler.createRequest(destinationId);
        return (await this.#sendRequest(destinationId, request)) ?? result;
    }

    async requestConnectToAccessPoint(
        destinationId: NodeId,
        ssid: Uint8Array,
        password: Uint8Array,
    ): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.ConnectToAccessPoint);
        const [request, result] = await handler.createRequest(destinationId, ssid, password);
        return (await this.#sendRequest(destinationId, request)) ?? result;
    }

    async requestStartServer(destinationId: NodeId, port: number): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.StartServer);
        const [request, result] = await handler.createRequest(destinationId, port);
        return (await this.#sendRequest(destinationId, request)) ?? result;
    }

    async requestSendHello(destinationId: NodeId, targetAddress: Address, linkCost: Cost): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.SendHello);
        const [request, result] = await handler.createRequest(destinationId, targetAddress, linkCost);
        return (await this.#sendRequest(destinationId, request)) ?? result;
    }

    async requestSendGoodbye(destinationId: NodeId, targetNode: NodeId): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.SendGoodbye);
        const [request, result] = await handler.createRequest(destinationId, targetNode);
        return (await this.#sendRequest(destinationId, request)) ?? result;
    }
}
