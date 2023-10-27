export type { RpcServer } from "./handler";
import { ReactiveService, RoutingFrame } from "@core/net/routing";
import { FrameType, Procedure, RpcRequest, RpcResponse, RpcStatus, deserializeFrame } from "../frame";
import * as RoutingNeighborSendHello from "./routingNeighborSendHello";
import * as RoutingNeighborSendGoodbye from "./routingNeighborSendGoodbye";
import * as RoutingNeighborGetNeighborList from "./routingNeighborGetNeighborList";
import { BufferReader } from "@core/net/buffer";
import { RpcServer } from "./handler";

const createClients = ({ reactiveService }: { reactiveService: ReactiveService }) => {
    return {
        [Procedure.RoutingNeighborSendHello]: new RoutingNeighborSendHello.Client({ reactiveService }),
        [Procedure.RoutingNeighborSendGoodbye]: new RoutingNeighborSendGoodbye.Client({ reactiveService }),
        [Procedure.RoutingNeighborGetNeighborList]: new RoutingNeighborGetNeighborList.Client({ reactiveService }),
    } as const;
};

type Clients = ReturnType<typeof createClients>;
type PickClient<P extends Procedure> = P extends keyof Clients ? Clients[P] : undefined;

const handleNotSupported = (request: RpcRequest): RpcResponse => ({
    frameType: FrameType.Response,
    procedure: request.procedure,
    frameId: request.frameId,
    status: RpcStatus.NotSupported,
    senderId: request.senderId,
    targetId: request.targetId,
    bodyReader: new BufferReader(new Uint8Array(0)),
});

export class ProcedureHandler {
    #clients: Clients;
    #servers: Map<Procedure, RpcServer> = new Map();

    constructor(args: { reactiveService: ReactiveService }) {
        this.#clients = createClients(args);

        this.#servers.set(Procedure.RoutingNeighborSendHello, new RoutingNeighborSendHello.Server(args));
        this.#servers.set(Procedure.RoutingNeighborSendGoodbye, new RoutingNeighborSendGoodbye.Server(args));
        this.#servers.set(Procedure.RoutingNeighborGetNeighborList, new RoutingNeighborGetNeighborList.Server(args));
    }

    getClient<P extends Procedure>(procedure: P): PickClient<P> {
        if (procedure in this.#clients) {
            return (this.#clients as any)[procedure] as PickClient<P>;
        } else {
            return undefined as PickClient<P>;
        }
    }

    handleFrame(frame: RoutingFrame): Promise<RpcResponse> | undefined {
        const rpcFrame = deserializeFrame(frame);

        if (rpcFrame.frameType === FrameType.Request) {
            const server = this.#servers.get(rpcFrame.procedure);
            return server?.handleRequest(rpcFrame) ?? Promise.resolve(handleNotSupported(rpcFrame));
        }

        const client = this.getClient(rpcFrame.procedure);
        if (client !== undefined) {
            client.handleResponse(rpcFrame);
        }
    }

    addServer(procedure: Procedure, handler: RpcServer): void {
        if (procedure in this.#clients) {
            throw new Error(`Handler for procedure ${procedure} already exists`);
        }
        (this.#clients as any)[procedure] = handler;
    }
}
