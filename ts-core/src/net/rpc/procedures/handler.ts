import { RpcRequest, RpcResponse } from "../frame";
import { RpcResult } from "../request";

export class RpcIgnoreRequest {}

export interface RpcServer {
    handleRequest(request: RpcRequest): Promise<RpcResponse | RpcIgnoreRequest>;
}

export interface RpcClient<T> {
    createRequest(...args: readonly unknown[]): Promise<[RpcRequest, Promise<RpcResult<T>>]>;
    handleResponse(response: RpcResponse): void;
}
