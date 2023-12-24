import { LocalNodeService } from "@core/net/local";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RpcClient, RpcIgnoreRequest, RpcRequestContext, RpcServer } from "../handler";
import { Cost, Destination } from "@core/net/node";
import { RequestManager, RpcResult } from "../../request";

export class Server implements RpcServer {
    #localNodeService: LocalNodeService;

    constructor(args: { localNodeService: LocalNodeService }) {
        this.#localNodeService = args.localNodeService;
    }

    async handleRequest(request: RpcRequest, ctx: RpcRequestContext): Promise<RpcResponse | RpcIgnoreRequest> {
        const cost = Cost.deserialize(request.bodyReader);
        if (cost.isErr()) {
            return ctx.createResponse({ status: RpcStatus.BadArgument });
        }
        await this.#localNodeService.setCost(cost.unwrap());

        return ctx.createResponse({ status: RpcStatus.Success });
    }
}

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.SetCost, localNodeService });
    }

    createRequest(destination: Destination, cost: Cost): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        return this.#requestManager.createRequest(destination, cost);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
