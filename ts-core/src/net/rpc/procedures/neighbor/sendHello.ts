import { Address } from "@core/net/link";
import { Cost, LocalNodeService, NodeId } from "@core/net/node";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { RpcClient, RpcServer } from "../handler";
import { Procedure, RpcRequest, RpcResponse, RpcStatus, createResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { DeserializeResult } from "@core/serde";
import { NeighborService } from "@core/net/neighbor";

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
    #neighborService: NeighborService;

    constructor(args: { neighborService: NeighborService }) {
        this.#neighborService = args.neighborService;
    }

    async handleRequest(request: RpcRequest): Promise<RpcResponse> {
        const param = Params.deserialize(request.bodyReader);
        if (param.isErr()) {
            return createResponse(request, { status: RpcStatus.BadArgument });
        }

        const { address, cost } = param.unwrap();
        const result = await this.#neighborService.sendHello(address, cost);
        if (result.isErr()) {
            return createResponse(request, { status: RpcStatus.Failed });
        }

        return createResponse(request, { status: RpcStatus.Success });
    }
}

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.SendHello, localNodeService });
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
        this.#requestManager.resolveVoid(response);
    }
}
