import { Address, MediaPortNumber } from "@core/net/link";
import { Cost, Destination } from "@core/net/node";
import { RpcRequestContext, RpcClient, RpcServer } from "../handler";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { ObjectSerdeable, OptionalSerdeable } from "@core/serde";
import { NeighborService } from "@core/net/neighbor";
import { LocalNodeService } from "@core/net/local";

const paramSerdeable = new ObjectSerdeable({
    address: Address.serdeable,
    cost: Cost.serdeable,
    mediaPort: new OptionalSerdeable(MediaPortNumber.serdeable),
});

export class Server implements RpcServer {
    #neighborService: NeighborService;

    constructor(args: { neighborService: NeighborService }) {
        this.#neighborService = args.neighborService;
    }

    async handleRequest(request: RpcRequest, ctx: RpcRequestContext): Promise<RpcResponse> {
        const param = paramSerdeable.deserializer().deserialize(request.bodyReader);
        if (param.isErr()) {
            return ctx.createResponse({ status: RpcStatus.BadArgument });
        }

        const { address, cost } = param.unwrap();
        const result = await this.#neighborService.sendHello(address, cost);
        if (result.isErr()) {
            return ctx.createResponse({ status: RpcStatus.Failed });
        }

        return ctx.createResponse({ status: RpcStatus.Success });
    }
}

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.SendHello, localNodeService });
    }

    createRequest(
        destination: Destination,
        targetAddress: Address,
        linkCost: Cost,
        mediaPort: MediaPortNumber | undefined,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = paramSerdeable.serializer({ address: targetAddress, cost: linkCost, mediaPort });
        return this.#requestManager.createRequest(destination, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
