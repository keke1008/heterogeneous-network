import { Address, LinkService } from "@core/net/link";
import { RpcRequestContext, RpcClient, RpcIgnoreRequest, RpcServer } from "../handler";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { Destination, NodeId } from "@core/net/node";
import { ObjectSerdeable, SerdeableValue, VectorSerdeable } from "@core/serde";
import { LocalNodeService } from "@core/net/local";

const paramSerdeable = new ObjectSerdeable({ targetId: NodeId.serdeable });
const resultSerdeable = new ObjectSerdeable({
    addresses: new VectorSerdeable(Address.serdeable),
});
type Result = SerdeableValue<typeof resultSerdeable>;

export class Server implements RpcServer {
    #linkService: LinkService;
    #localNodeService: LocalNodeService;

    constructor({ linkService, localNodeService }: { linkService: LinkService; localNodeService: LocalNodeService }) {
        this.#linkService = linkService;
        this.#localNodeService = localNodeService;
    }

    async handleRequest(request: RpcRequest, ctx: RpcRequestContext): Promise<RpcResponse | RpcIgnoreRequest> {
        const param = paramSerdeable.deserializer().deserialize(request.bodyReader);
        if (param.isErr()) {
            return ctx.createResponse({ status: RpcStatus.BadArgument });
        }

        const { targetId } = param.unwrap();
        if (!this.#localNodeService.id?.equals(targetId)) {
            return new RpcIgnoreRequest();
        }

        return ctx.createResponse({
            status: RpcStatus.Success,
            serializer: resultSerdeable.serializer({
                addresses: this.#linkService.getLocalAddresses(),
            }),
        });
    }
}

export class Client implements RpcClient<Result> {
    #requestManager: RequestManager<Result>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.ResolveAddress, localNodeService });
    }

    createRequest(targetId: NodeId): Promise<[RpcRequest, Promise<RpcResult<Result>>]> {
        return this.#requestManager.createRequest(Destination.broadcast(), paramSerdeable.serializer({ targetId }));
    }

    handleResponse(response: RpcResponse): void {
        if (response.status !== RpcStatus.Success) {
            this.#requestManager.resolveFailure(response.requestId, response.status);
            return;
        }

        const result = resultSerdeable.deserializer().deserialize(response.bodyReader);
        if (result.isOk()) {
            this.#requestManager.resolveSuccess(response.requestId, result.unwrap());
        } else {
            this.#requestManager.resolveFailure(response.requestId, RpcStatus.BadResponseFormat);
        }
    }
}
