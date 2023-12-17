export type { RpcServer } from "./handler";
export { BlinkOperation } from "./debug/blink";
export { MediaInfo } from "./media/getMediaList";

import { RoutingFrame } from "@core/net/routing";
import { FrameType, Procedure, RpcRequest, RpcResponse, RpcStatus, deserializeFrame } from "../frame";
import { BufferReader } from "@core/net/buffer";
import { RpcIgnoreRequest, RpcRequestContext, RpcServer } from "./handler";
import { NeighborService } from "@core/net/neighbor";
import { LocalNodeService } from "@core/net/node";
import { LinkService } from "@core/net/link";

import * as Blink from "./debug/blink";
import * as GetMediaList from "./media/getMediaList";
import * as StartServer from "./wifi/startServer";
import * as ConnectToAccessPoint from "./wifi/connectToAccessPoint";
import * as SendHello from "./neighbor/sendHello";
import * as SendGoodbye from "./neighbor/sendGoodbye";
import * as ResolveAddress from "./address/resolveAddress";
import * as GetVRouters from "./vrouter/getVRouters";
import * as CreateVRouter from "./vrouter/createVRouter";
import * as DeleteVRouter from "./vrouter/deleteVRouter";

const createClients = (args: { localNodeService: LocalNodeService }) => {
    return {
        [Procedure.Blink]: new Blink.Client(args),
        [Procedure.GetMediaList]: new GetMediaList.Client(args),
        [Procedure.SendHello]: new SendHello.Client(args),
        [Procedure.SendGoodbye]: new SendGoodbye.Client(args),
        [Procedure.ConnectToAccessPoint]: new ConnectToAccessPoint.Client(args),
        [Procedure.StartServer]: new StartServer.Client(args),
        [Procedure.ResolveAddress]: new ResolveAddress.Client(args),
        [Procedure.GetVRouters]: new GetVRouters.Client(args),
        [Procedure.CreateVRouter]: new CreateVRouter.Client(args),
        [Procedure.DeleteVRouter]: new DeleteVRouter.Client(args),
    } as const;
};

type Clients = ReturnType<typeof createClients>;
type PickClient<P extends Procedure> = P extends keyof Clients ? Clients[P] : undefined;

const handleNotSupported = (request: RpcRequest): RpcResponse => ({
    frameType: FrameType.Response,
    procedure: request.procedure,
    requestId: request.requestId,
    status: RpcStatus.NotSupported,
    bodyReader: new BufferReader(new Uint8Array(0)),
});

export class ProcedureHandler {
    #clients: Clients;
    #servers: Map<Procedure, RpcServer> = new Map();

    constructor(args: {
        linkService: LinkService;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
    }) {
        this.#clients = createClients(args);

        this.#servers.set(Procedure.SendHello, new SendHello.Server(args));
        this.#servers.set(Procedure.SendGoodbye, new SendGoodbye.Server(args));
        this.#servers.set(Procedure.ResolveAddress, new ResolveAddress.Server(args));
    }

    getClient<P extends Procedure>(procedure: P): PickClient<P> {
        if (procedure in this.#clients) {
            return this.#clients[procedure as keyof Clients] as PickClient<P>;
        } else {
            return undefined as PickClient<P>;
        }
    }

    async handleReceivedFrame(frame: RoutingFrame): Promise<RpcResponse | undefined> {
        const deserializedRpcFrame = deserializeFrame(frame);
        if (deserializedRpcFrame.isErr()) {
            console.warn("Failed to deserialize frame", deserializedRpcFrame.unwrapErr());
            return undefined;
        }

        const rpcFrame = deserializedRpcFrame.unwrap();
        if (rpcFrame.frameType === FrameType.Request) {
            const server = this.#servers.get(rpcFrame.procedure);
            if (server === undefined) {
                return handleNotSupported(rpcFrame);
            }

            const ctx = new RpcRequestContext({ request: rpcFrame });
            const response = await server.handleRequest(rpcFrame, ctx);
            return response instanceof RpcIgnoreRequest ? undefined : response;
        }

        const client = this.getClient(rpcFrame.procedure);
        client?.handleResponse(rpcFrame);
    }

    addServer(procedure: Procedure, handler: RpcServer): void {
        if (this.#servers.has(procedure)) {
            throw new Error(`Handler for procedure ${procedure} already exists`);
        }
        this.#servers.set(procedure, handler);
    }
}
