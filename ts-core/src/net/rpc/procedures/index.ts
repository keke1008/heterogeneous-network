export type { RpcServer } from "./handler";
export { BlinkOperation } from "./debug/blink";
export { MediaInfo } from "./media/getMediaList";

import { ReactiveService, RoutingFrame } from "@core/net/routing";
import { FrameType, Procedure, RpcRequest, RpcResponse, RpcStatus, deserializeFrame } from "../frame";
import { BufferReader } from "@core/net/buffer";
import { RpcServer } from "./handler";

import * as Blink from "./debug/blink";
import * as GetMediaList from "./media/getMediaList";
import * as StartServer from "./wifi/startServer";
import * as ConnectToAccessPoint from "./wifi/connectToAccessPoint";
import * as SendHello from "./neighbor/sendHello";
import * as SendGoodbye from "./neighbor/sendGoodbye";

const createClients = ({ reactiveService }: { reactiveService: ReactiveService }) => {
    return {
        [Procedure.Blink]: new Blink.Client({ reactiveService }),
        [Procedure.GetMediaList]: new GetMediaList.Client({ reactiveService }),
        [Procedure.SendHello]: new SendHello.Client({ reactiveService }),
        [Procedure.SendGoodbye]: new SendGoodbye.Client({ reactiveService }),
        [Procedure.ConnectToAccessPoint]: new ConnectToAccessPoint.Client({ reactiveService }),
        [Procedure.StartServer]: new StartServer.Client({ reactiveService }),
    } as const;
};

type Clients = ReturnType<typeof createClients>;
type PickClient<P extends Procedure> = P extends keyof Clients ? Clients[P] : undefined;

const handleNotSupported = (request: RpcRequest): RpcResponse => ({
    frameType: FrameType.Response,
    procedure: request.procedure,
    requestId: request.requestId,
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

        this.#servers.set(Procedure.SendHello, new SendHello.Server(args));
        this.#servers.set(Procedure.SendGoodbye, new SendGoodbye.Server(args));
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
            return server?.handleRequest(rpcFrame) ?? handleNotSupported(rpcFrame);
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
