import { Address } from "@core/net/link";
import { Cost, NodeId } from "@core/net/node";
import { ReactiveService } from "@core/net/routing";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { RpcClient, RpcServer } from "../handler";
import { Procedure, RpcRequest, RpcResponse, RpcStatus, createResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { DeserializeResult } from "@core/serde";

class Params {
    address: Address;
    cost: Cost;

    constructor(args: { address: Address; cost: Cost }) {
        this.address = args.address;
        this.cost = args.cost;
    }

    static deserialize(reader: BufferReader): DeserializeResult<Params> {
        return Address.deserialize(reader).andThen((address) => {
            return Cost.deserialize(reader).map((cost) => {
                return new Params({ address, cost });
            });
        });
    }

    serialize(writer: BufferWriter) {
        this.address.serialize(writer);
        this.cost.serialize(writer);
    }

    serializedLength(): number {
        return this.address.serializedLength() + this.cost.serializedLength();
    }
}

export class Server implements RpcServer {
    #reactiveService: ReactiveService;

    constructor(args: { reactiveService: ReactiveService }) {
        this.#reactiveService = args.reactiveService;
    }

    async handleRequest(request: RpcRequest): Promise<RpcResponse> {
        const param = Params.deserialize(request.bodyReader);
        if (param.isErr()) {
            return createResponse(request, { status: RpcStatus.BadArgument });
        }

        const { address, cost } = param.unwrap();
        const result = await this.#reactiveService.requestHello(address, cost);
        if (result.isErr()) {
            return createResponse(request, { status: RpcStatus.Failed });
        }

        return createResponse(request, { status: RpcStatus.Success });
    }
}

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ reactiveService }: { reactiveService: ReactiveService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.SendHello, reactiveService });
    }

    createRequest(
        destinationId: NodeId,
        targetAddress: Address,
        linkCost: Cost,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = new Params({ address: targetAddress, cost: linkCost });
        return this.#requestManager.createRequest(destinationId, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response.requestId);
    }
}
