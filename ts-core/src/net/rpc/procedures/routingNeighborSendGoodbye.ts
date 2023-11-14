import { NodeId, ReactiveService } from "@core/net/routing";
import { RpcClient, RpcServer } from "./handler";
import { RequestManager, RpcResult } from "../request";
import { FrameType, Procedure, RpcRequest, RpcResponse, RpcStatus, createResponse } from "../frame";
import { BufferReader, BufferWriter } from "@core/net/buffer";

class RequestFrameBody {
    targetId: NodeId;

    constructor(args: { targetNode: NodeId }) {
        this.targetId = args.targetNode;
    }

    static deserialize(reader: BufferReader): RequestFrameBody {
        const targetNode = NodeId.deserialize(reader);
        return new RequestFrameBody({ targetNode });
    }

    serialize(writer: BufferWriter): void {
        this.targetId.serialize(writer);
    }

    serializedLength(): number {
        return this.targetId.serializedLength();
    }
}

export class Server implements RpcServer {
    #reactiveService: ReactiveService;

    constructor(args: { reactiveService: ReactiveService }) {
        this.#reactiveService = args.reactiveService;
    }

    async handleRequest(request: RpcRequest): Promise<RpcResponse> {
        const body = RequestFrameBody.deserialize(request.bodyReader);
        if (request.bodyReader.remainingLength() !== 0) {
            return createResponse(request, RpcStatus.BadArgument);
        }

        this.#reactiveService.requestGoodbye(body.targetId);
        return createResponse(request, RpcStatus.Success);
    }
}

export class Client implements RpcClient<void> {
    #reactiveService: ReactiveService;
    #requestManager: RequestManager<void> = new RequestManager();

    constructor(args: { reactiveService: ReactiveService }) {
        this.#reactiveService = args.reactiveService;
    }

    createRequest(destinationId: NodeId, targetNode: NodeId): [RpcRequest, Promise<RpcResult<void>>] {
        const [frameId, promise] = this.#requestManager.request();
        const body = new RequestFrameBody({ targetNode });
        const bodyWriter = new BufferWriter(new Uint8Array(body.serializedLength()));
        body.serialize(bodyWriter);

        const request: RpcRequest = {
            frameType: FrameType.Request,
            procedure: Procedure.RoutingNeighborSendGoodbye,
            frameId,
            senderId: this.#reactiveService.localId(),
            targetId: destinationId,
            bodyReader: new BufferReader(bodyWriter.unwrapBuffer()),
        };
        return [request, promise];
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response.frameId, response.status);
    }
}
