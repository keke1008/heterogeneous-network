import { Address, MediaPortNumber } from "@core/net/link";
import { Cost, Destination } from "@core/net/node";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { RpcRequestContext, RpcClient, RpcServer } from "../handler";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { DeserializeOptional, DeserializeResult, SerializeOptional } from "@core/serde";
import { NeighborService } from "@core/net/neighbor";
import { LocalNodeService } from "@core/net/local";

class Params {
    address: Address;
    cost: Cost;
    mediaPort: MediaPortNumber | undefined;

    constructor(args: { address: Address; cost: Cost; mediaPort: MediaPortNumber | undefined }) {
        this.address = args.address;
        this.cost = args.cost;
        this.mediaPort = args.mediaPort;
    }

    static deserialize(reader: BufferReader): DeserializeResult<Params> {
        return Address.deserialize(reader).andThen((address) => {
            return Cost.deserialize(reader).andThen((cost) => {
                return new DeserializeOptional(MediaPortNumber).deserialize(reader).map((mediaPort) => {
                    return new Params({ address, cost, mediaPort });
                });
            });
        });
    }

    serialize(writer: BufferWriter) {
        this.address.serialize(writer);
        this.cost.serialize(writer);
        new SerializeOptional(this.mediaPort).serialize(writer);
    }

    serializedLength(): number {
        return (
            this.address.serializedLength() +
            this.cost.serializedLength() +
            new SerializeOptional(this.mediaPort).serializedLength()
        );
    }
}

export class Server implements RpcServer {
    #neighborService: NeighborService;

    constructor(args: { neighborService: NeighborService }) {
        this.#neighborService = args.neighborService;
    }

    async handleRequest(request: RpcRequest, ctx: RpcRequestContext): Promise<RpcResponse> {
        const param = Params.deserialize(request.bodyReader);
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
        const body = new Params({ address: targetAddress, cost: linkCost, mediaPort });
        return this.#requestManager.createRequest(destination, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
