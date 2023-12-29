import { FrameType, RpcRequest, RpcResponse, RpcStatus } from "../frame";
import { RpcResult } from "../request";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Serializer } from "@core/serde";

export class RpcIgnoreRequest {}

export class RpcRequestContext {
    #request: RpcRequest;

    constructor({ request }: { request: RpcRequest }) {
        this.#request = request;
    }

    async createResponse(args: { status: RpcStatus; serializer?: Serializer }): Promise<RpcResponse> {
        let bodyReader;
        if (args.serializer !== undefined) {
            const writer = new BufferWriter(new Uint8Array(args.serializer.serializedLength()));
            args.serializer.serialize(writer);
            bodyReader = new BufferReader(writer.unwrapBuffer());
        } else {
            bodyReader = new BufferReader(new Uint8Array(0));
        }

        return {
            frameType: FrameType.Response,
            procedure: this.#request.procedure,
            requestId: this.#request.requestId,
            status: args.status,
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
