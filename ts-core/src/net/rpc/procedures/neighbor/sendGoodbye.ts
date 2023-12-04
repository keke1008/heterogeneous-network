import { NodeId } from "@core/net/node";
import { ReactiveService } from "@core/net/routing";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { RpcClient, RpcServer } from "../handler";
import { Procedure, RpcRequest, RpcResponse, RpcStatus, createResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { DeserializeResult } from "@core/serde";

class Params {
    nodeId: NodeId;

    constructor(args: { nodeId: NodeId }) {
        this.nodeId = args.nodeId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<Params> {
        return NodeId.deserialize(reader).map((nodeId) => new Params({ nodeId }));
    }

    serialize(writer: BufferWriter) {
        this.nodeId.serialize(writer);
    }

    serializedLength(): number {
        return this.nodeId.serializedLength();
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

        const { nodeId } = param.unwrap();
        const result = await this.#reactiveService.requestGoodbye(nodeId);
        if (result.isErr()) {
            return createResponse(request, { status: RpcStatus.Failed });
        }

        return createResponse(request, { status: RpcStatus.Success });
    }
}

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ reactiveService }: { reactiveService: ReactiveService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.SendGoodbye, reactiveService });
    }

    createRequest(destinationId: NodeId, targetNodeId: NodeId): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = new Params({ nodeId: targetNodeId });
        return this.#requestManager.createRequest(destinationId, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response.requestId);
    }
}
