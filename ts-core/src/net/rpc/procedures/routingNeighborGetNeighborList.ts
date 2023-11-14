import { BufferReader, BufferWriter } from "@core/net/buffer";
import { NodeId, ReactiveService } from "@core/net/routing";
import { RpcClient, RpcServer } from "./handler";
import { FrameType, Procedure, RpcRequest, RpcResponse, RpcStatus, createResponse } from "../frame";
import { RequestManager, RpcResult } from "../request";

class ResponseFrameBody {
    neighborList: NodeId[];

    constructor(args: { neighborList: NodeId[] }) {
        this.neighborList = args.neighborList;
    }

    static deserialize(reader: BufferReader): ResponseFrameBody {
        const neighborList = [];
        while (reader.remainingLength() > 0) {
            neighborList.push(NodeId.deserialize(reader));
        }
        return new ResponseFrameBody({ neighborList });
    }

    serialize(writer: BufferWriter): void {
        for (const neighbor of this.neighborList) {
            neighbor.serialize(writer);
        }
    }

    serializedLength(): number {
        return this.neighborList.reduce((acc, neighbor) => acc + neighbor.serializedLength(), 0);
    }
}

export class Server implements RpcServer {
    #reactiveService: ReactiveService;

    constructor(args: { reactiveService: ReactiveService }) {
        this.#reactiveService = args.reactiveService;
    }

    async handleRequest(request: RpcRequest): Promise<RpcResponse> {
        if (request.bodyReader.remainingLength() !== 0) {
            return createResponse(request, RpcStatus.BadArgument);
        }

        const neighborList = this.#reactiveService.getNeighborList();
        const body = new ResponseFrameBody({ neighborList: neighborList.map((neighbor) => neighbor.id) });
        const bodyWriter = new BufferWriter(new Uint8Array(body.serializedLength()));
        body.serialize(bodyWriter);
        return createResponse(request, RpcStatus.Success, new BufferReader(bodyWriter.unwrapBuffer()));
    }
}

export class Client implements RpcClient<NodeId[]> {
    #reactiveService: ReactiveService;
    #requestManager: RequestManager<NodeId[]> = new RequestManager();

    constructor(args: { reactiveService: ReactiveService }) {
        this.#reactiveService = args.reactiveService;
    }

    createRequest(destinationId: NodeId): [RpcRequest, Promise<RpcResult<NodeId[]>>] {
        const [frameId, promise] = this.#requestManager.request();
        const bodyWriter = new BufferWriter(new Uint8Array(0));
        const request: RpcRequest = {
            frameType: FrameType.Request,
            procedure: Procedure.RoutingNeighborGetNeighborList,
            frameId,
            senderId: this.#reactiveService.localId(),
            targetId: destinationId,
            bodyReader: new BufferReader(bodyWriter.unwrapBuffer()),
        };
        return [request, promise];
    }

    handleResponse(response: RpcResponse): void {
        if (response.status !== RpcStatus.Success) {
            this.#requestManager.resolveFailure(response.frameId, response.status);
            return;
        }

        const body = ResponseFrameBody.deserialize(response.bodyReader);
        this.#requestManager.resolveSuccess(response.frameId, body.neighborList);
    }
}
