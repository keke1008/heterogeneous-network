import { Address, LinkService } from "@core/net/link";
import { RpcRequestContext, RpcClient, RpcIgnoreRequest, RpcServer } from "../handler";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { Destination, NodeId } from "@core/net/node";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { DeserializeResult, DeserializeVector, SerializeVector } from "@core/serde";
import { LocalNodeService } from "@core/net/local";

class Param {
    targetId: NodeId;

    constructor(opt: { targetId: NodeId }) {
        this.targetId = opt.targetId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<Param> {
        return NodeId.deserialize(reader).map((targetId) => {
            return new Param({ targetId });
        });
    }

    serialize(writer: BufferWriter): void {
        this.targetId.serialize(writer);
    }

    serializedLength(): number {
        return this.targetId.serializedLength();
    }
}

export class Result {
    addresses: Address[];

    constructor(opt: { addresses: Address[] }) {
        this.addresses = opt.addresses;
    }

    static deserialize(reader: BufferReader): DeserializeResult<Result> {
        return new DeserializeVector(Address).deserialize(reader).map((addresses) => {
            return new Result({ addresses });
        });
    }

    serialize(writer: BufferWriter): void {
        new SerializeVector(this.addresses).serialize(writer);
    }

    serializedLength(): number {
        return new SerializeVector(this.addresses).serializedLength();
    }
}

export class Server implements RpcServer {
    #linkService: LinkService;
    #localNodeService: LocalNodeService;

    constructor({ linkService, localNodeService }: { linkService: LinkService; localNodeService: LocalNodeService }) {
        this.#linkService = linkService;
        this.#localNodeService = localNodeService;
    }

    async handleRequest(request: RpcRequest, ctx: RpcRequestContext): Promise<RpcResponse | RpcIgnoreRequest> {
        const param = Param.deserialize(request.bodyReader);
        if (param.isErr()) {
            return ctx.createResponse({ status: RpcStatus.BadArgument });
        }

        const { targetId } = param.unwrap();
        if (!this.#localNodeService.id?.equals(targetId)) {
            return new RpcIgnoreRequest();
        }

        const result = new Result({ addresses: this.#linkService.getLocalAddresses() });
        return ctx.createResponse({ status: RpcStatus.Success, serializable: result });
    }
}

export class Client implements RpcClient<Result> {
    #requestManager: RequestManager<Result>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.ResolveAddress, localNodeService });
    }

    createRequest(targetId: NodeId): Promise<[RpcRequest, Promise<RpcResult<Result>>]> {
        const body = new Param({ targetId });
        return this.#requestManager.createRequest(Destination.broadcast(), body);
    }

    handleResponse(response: RpcResponse): void {
        if (response.status !== RpcStatus.Success) {
            this.#requestManager.resolveFailure(response.requestId, response.status);
            return;
        }

        const result = Result.deserialize(response.bodyReader);
        if (result.isOk()) {
            this.#requestManager.resolveSuccess(response.requestId, result.unwrap());
        } else {
            this.#requestManager.resolveFailure(response.requestId, RpcStatus.BadResponseFormat);
        }
    }
}
