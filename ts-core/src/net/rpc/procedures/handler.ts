import { NodeId } from "../../node";
import { RpcRequest, RpcResponse } from "../frame";
import { RpcResult } from "../request";

export interface RpcServer {
    handleRequest(request: RpcRequest): Promise<RpcResponse>;
}

export interface RpcClient<T> {
    createRequest(destinationId: NodeId, ...args: unknown[]): Promise<[RpcRequest, Promise<RpcResult<T>>]>;
    handleResponse(response: RpcResponse): void;
}
