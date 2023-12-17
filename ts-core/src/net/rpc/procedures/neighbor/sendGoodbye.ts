import { Destination, LocalNodeService, NodeId } from "@core/net/node";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { RpcRequestContext, RpcClient, RpcServer } from "../handler";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { DeserializeResult } from "@core/serde";
import { NeighborService } from "@core/net/neighbor";

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
    #neighborService: NeighborService;

    constructor(args: { neighborService: NeighborService }) {
        this.#neighborService = args.neighborService;
    }

    async handleRequest(request: RpcRequest, ctx: RpcRequestContext): Promise<RpcResponse> {
        const param = Params.deserialize(request.bodyReader);
        if (param.isErr()) {
            return ctx.createResponse({ status: RpcStatus.BadArgument });
        }

        const { nodeId } = param.unwrap();
        const result = await this.#neighborService.sendGoodbye(nodeId);
        if (result.isErr()) {
            return ctx.createResponse({ status: RpcStatus.Failed });
        }

        return ctx.createResponse({ status: RpcStatus.Success });
    }
}

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.SendGoodbye, localNodeService });
    }

    createRequest(destination: Destination, targetNodeId: NodeId): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = new Params({ nodeId: targetNodeId });
        return this.#requestManager.createRequest(destination, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
