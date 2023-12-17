import { LocalNodeService } from "@core/net/node";
import { FrameType, RpcRequest, RpcResponse, RpcStatus } from "../frame";
import { RpcResult } from "../request";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Serializable } from "@core/serde";

export class RpcIgnoreRequest {}

export class RpcRequestContext {
    #request: RpcRequest;
    #localNodeService: LocalNodeService;

    constructor({ request, localNodeService }: { request: RpcRequest; localNodeService: LocalNodeService }) {
        this.#request = request;
        this.#localNodeService = localNodeService;
    }

    async createResponse(args: { status: RpcStatus; serializable?: Serializable }): Promise<RpcResponse> {
        const localId = await this.#localNodeService.getId();
        let bodyReader;
        if (args.serializable !== undefined) {
            const writer = new BufferWriter(new Uint8Array(args.serializable.serializedLength()));
            args.serializable.serialize(writer);
            bodyReader = new BufferReader(writer.unwrapBuffer());
        } else {
            bodyReader = new BufferReader(new Uint8Array(0));
        }

        return {
            frameType: FrameType.Response,
            procedure: this.#request.procedure,
            requestId: this.#request.requestId,
            status: args.status,
            clientId: localId, // `this.#request.targetId`はブロードキャストの場合があるため使用しない
            serverId: this.#request.clientId,
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
