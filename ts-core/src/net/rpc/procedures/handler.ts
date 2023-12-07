import { LocalNodeService } from "@core/net/node";
import { FrameType, RpcRequest, RpcResponse, RpcStatus } from "../frame";
import { RpcResult } from "../request";
import { BufferReader } from "@core/net/buffer";

export class RpcIgnoreRequest {}

export class RpcRequestContext {
    #request: RpcRequest;
    #localNodeService: LocalNodeService;

    constructor({ request, localNodeService }: { request: RpcRequest; localNodeService: LocalNodeService }) {
        this.#request = request;
        this.#localNodeService = localNodeService;
    }

    async createResponse(args: { status: RpcStatus; reader?: BufferReader }): Promise<RpcResponse> {
        const localId = await this.#localNodeService.getId();
        const bodyReader = args.reader ?? new BufferReader(new Uint8Array(0));
        return {
            frameType: FrameType.Response,
            procedure: this.#request.procedure,
            requestId: this.#request.requestId,
            status: args.status,
            senderId: localId, // `this.#request.targetId`はブロードキャストの場合があるため使用しない
            targetId: this.#request.senderId,
            bodyReader,
        };
    }
}

export interface RpcServer {
    handleRequest(request: RpcRequest, ctx: RpcRequestContext): Promise<RpcResponse | RpcIgnoreRequest>;
}

export interface RpcClient<T> {
    createRequest(...args: readonly unknown[]): Promise<[RpcRequest, Promise<RpcResult<T>>]>;
    handleResponse(response: RpcResponse): void;
}
