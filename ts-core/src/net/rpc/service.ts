import { Address, LinkService, MediaPortNumber, Protocol } from "../link";
import { LocalNodeService } from "../local";
import { NeighborService } from "../neighbor";
import { Cost, Destination } from "../node";
import { OptionalClusterId } from "../node/clusterId";
import { RoutingFrame, RoutingSocket } from "../routing";
import { RoutingService } from "../routing/service";
import { MAX_FRAME_ID_CACHE_SIZE } from "./constants";
import { Procedure, RpcRequest, RpcStatus, serializeFrame } from "./frame";
import {
    RpcServer,
    ProcedureHandler,
    BlinkOperation,
    MediaInfo,
    SetEthernetIpAddressParam,
    SetEthernetSubnetMaskParam,
} from "./procedures";
import { VRouter } from "./procedures/vrouter/getVRouters";
import { RpcResult } from "./request";

export class RpcService {
    #handler: ProcedureHandler;
    #socket: RoutingSocket;

    constructor(args: {
        linkService: LinkService;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        routingService: RoutingService;
    }) {
        this.#handler = new ProcedureHandler({
            linkService: args.linkService,
            neighborService: args.neighborService,
            localNodeService: args.localNodeService,
        });

        const linkSocket = args.linkService.open(Protocol.Rpc);
        this.#socket = new RoutingSocket({
            linkSocket,
            localNodeService: args.localNodeService,
            neighborService: args.neighborService,
            routingService: args.routingService,
            maxFrameIdCacheSize: MAX_FRAME_ID_CACHE_SIZE,
            includeLoopbackOnBroadcast: true,
        });
        this.#socket.onReceive((frame) => this.#handleReceive(frame));
    }

    async #handleReceive(frame: RoutingFrame): Promise<void> {
        const response = await this.#handler.handleReceivedFrame(frame);
        if (response !== undefined) {
            this.#socket.send(frame.source.intoDestination(), serializeFrame(response));
        }
    }

    async #sendRequest(request: RpcRequest): Promise<RpcResult<never> | undefined> {
        const sendResult = await this.#socket.send(request.server, serializeFrame(request));
        if (sendResult.isErr()) {
            return { status: RpcStatus.Unreachable };
        }
    }

    addServer(procedure: Procedure, server: RpcServer): void {
        this.#handler.addServer(procedure, server);
    }

    async requestBlink(destination: Destination, operation: BlinkOperation): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.Blink);
        const [request, result] = await handler.createRequest(destination, operation);
        return (await this.#sendRequest(request)) ?? result;
    }

    async requestGetMediaList(destination: Destination): Promise<RpcResult<MediaInfo[]>> {
        const handler = this.#handler.getClient(Procedure.GetMediaList);
        const [request, result] = await handler.createRequest(destination);
        return (await this.#sendRequest(request)) ?? result;
    }

    async requestConnectToAccessPoint(
        destination: Destination,
        ssid: Uint8Array,
        password: Uint8Array,
        mediaPort: MediaPortNumber,
    ): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.ConnectToAccessPoint);
        const [request, result] = await handler.createRequest(destination, ssid, password, mediaPort);
        return (await this.#sendRequest(request)) ?? result;
    }

    async requestStartServer(
        destination: Destination,
        port: number,
        mediaPort: MediaPortNumber,
    ): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.StartServer);
        const [request, result] = await handler.createRequest(destination, port, mediaPort);
        return (await this.#sendRequest(request)) ?? result;
    }

    async requestSetSeriaAddress(
        destination: Destination,
        portNumber: MediaPortNumber,
        address: Address,
    ): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.SetAddress);
        const [request, result] = await handler.createRequest(destination, portNumber, address);
        return (await this.#sendRequest(request)) ?? result;
    }

    async requestSetClusterId(destination: Destination, clusterId: OptionalClusterId): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.SetClusterId);
        const [request, result] = await handler.createRequest(destination, clusterId);
        return (await this.#sendRequest(request)) ?? result;
    }

    async requestSetCost(destination: Destination, cost: Cost): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.SetCost);
        const [request, result] = await handler.createRequest(destination, cost);
        return (await this.#sendRequest(request)) ?? result;
    }

    async requestSetEthernetIpAddress(
        destination: Destination,
        params: SetEthernetIpAddressParam,
    ): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.SetEthernetIpAddress);
        const [request, result] = await handler.createRequest(destination, params);
        return (await this.#sendRequest(request)) ?? result;
    }

    async requestSetEthernetSubnetMask(
        destination: Destination,
        params: SetEthernetSubnetMaskParam,
    ): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.SetEthernetSubnetMask);
        const [request, result] = await handler.createRequest(destination, params);
        return (await this.#sendRequest(request)) ?? result;
    }

    async requestSendHello(
        destination: Destination,
        targetAddress: Address,
        linkCost: Cost,
        mediaPort: MediaPortNumber | undefined,
    ): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.SendHello);
        const [request, result] = await handler.createRequest(destination, targetAddress, linkCost, mediaPort);
        return (await this.#sendRequest(request)) ?? result;
    }

    async requestGetVRouters(destination: Destination): Promise<RpcResult<VRouter[]>> {
        const handler = this.#handler.getClient(Procedure.GetVRouters);
        const [request, result] = await handler.createRequest(destination);
        return (await this.#sendRequest(request)) ?? result;
    }

    async requestCreateVRouter(destination: Destination): Promise<RpcResult<VRouter>> {
        const handler = this.#handler.getClient(Procedure.CreateVRouter);
        const [request, result] = await handler.createRequest(destination);
        return (await this.#sendRequest(request)) ?? result;
    }

    async requestDeleteVRouter(destination: Destination, port: number): Promise<RpcResult<void>> {
        const handler = this.#handler.getClient(Procedure.DeleteVRouter);
        const [request, result] = await handler.createRequest(destination, { port });
        return (await this.#sendRequest(request)) ?? result;
    }
}
