import { AddressError } from "@core/net/link";
import { NodeId, ReactiveService } from "@core/net/routing";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { RpcClient, RpcServer } from "../handler";
import { Procedure, RpcRequest, RpcResponse, RpcStatus, createResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { Result } from "oxide.ts";

class Params {
    nodeId: NodeId;

    constructor(args: { nodeId: NodeId }) {
        this.nodeId = args.nodeId;
    }

    static deserialize(reader: BufferReader): Result<Params, AddressError> {
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
        const result = this.#reactiveService.requestGoodbye(nodeId);
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

    createRequest(destinationId: NodeId, targetNodeId: NodeId): [RpcRequest, Promise<RpcResult<void>>] {
        const body = new Params({ nodeId: targetNodeId });
        return this.#requestManager.createRequest(destinationId, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response.frameId);
    }
}
