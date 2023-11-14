import { RpcStatus, RpcRequest, RpcResponse, createResponse, FrameType, Procedure } from "../frame";
import { RpcClient, RpcServer } from "./handler";
import { Cost, NodeId, ReactiveService } from "@core/net/routing";
import { Address } from "@core/net/link";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { RequestManager, RpcResult } from "../request";

class RequestFrameBody {
    targetAddress: Address;
    edgeCost: Cost;

    constructor(args: { targetAddress: Address; edgeCost: Cost }) {
        this.targetAddress = args.targetAddress;
        this.edgeCost = args.edgeCost;
    }

    static deserialize(reader: BufferReader): RequestFrameBody {
        const targetAddress = Address.deserialize(reader);
        const edgeCost = Cost.deserialize(reader);
        return new RequestFrameBody({ targetAddress, edgeCost });
    }

    serialize(writer: BufferWriter): void {
        this.targetAddress.serialize(writer);
        this.edgeCost.serialize(writer);
    }

    serializedLength(): number {
        return this.targetAddress.serializedLength() + this.edgeCost.serializedLength();
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

        this.#reactiveService.requestHello(body.targetAddress, body.edgeCost);
        return createResponse(request, RpcStatus.Success);
    }
}

export class Client implements RpcClient<void> {
    #reactiveService: ReactiveService;
    #requestManager: RequestManager<void> = new RequestManager();

    constructor(args: { reactiveService: ReactiveService }) {
        this.#reactiveService = args.reactiveService;
    }

    createRequest(
        destinationId: NodeId,
        targetAddress: Address,
        edgeCost: Cost,
    ): [RpcRequest, Promise<RpcResult<void>>] {
        const [frameId, promise] = this.#requestManager.request();
        const body = new RequestFrameBody({ targetAddress, edgeCost });
        const bodyWriter = new BufferWriter(new Uint8Array(body.serializedLength()));
        body.serialize(bodyWriter);

        const request: RpcRequest = {
            frameType: FrameType.Request,
            procedure: Procedure.RoutingNeighborSendHello,
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
