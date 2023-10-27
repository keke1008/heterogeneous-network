import { NodeId } from "@core/net/routing";
import { RpcRequest, RpcResponse } from "../frame";
import { RpcResult } from "../request";

export interface RpcServer {
    handleRequest(request: RpcRequest): Promise<RpcResponse>;
}

export interface RpcClient<T> {
    createRequest(destinationId: NodeId, ...args: unknown[]): [RpcRequest, Promise<RpcResult<T>>];
    handleResponse(response: RpcResponse): void;
}
