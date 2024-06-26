import { LocalNodeService } from "@core/net/local";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RpcClient, RpcIgnoreRequest, RpcRequestContext, RpcServer } from "../handler";
import { ClusterId, Destination } from "@core/net/node";
import { RequestManager, RpcResult } from "../../request";
import { OptionalClusterId } from "@core/net/node/clusterId";

export class Server implements RpcServer {
    #localNodeService: LocalNodeService;

    constructor(args: { localNodeService: LocalNodeService }) {
        this.#localNodeService = args.localNodeService;
    }

    async handleRequest(request: RpcRequest, ctx: RpcRequestContext): Promise<RpcResponse | RpcIgnoreRequest> {
        const clusterId = ClusterId.serdeable.deserializer().deserialize(request.bodyReader);
        if (clusterId.isErr()) {
            return ctx.createResponse({ status: RpcStatus.BadArgument });
        }
        await this.#localNodeService.setClusterId(clusterId.unwrap());

        return ctx.createResponse({ status: RpcStatus.Success });
    }
}

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.SetClusterId, localNodeService });
    }

    createRequest(
        destination: Destination,
        clusterId: OptionalClusterId,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        return this.#requestManager.createRequest(destination, ClusterId.serdeable.serializer(clusterId));
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
