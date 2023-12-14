import { LocalNodeService, NodeId } from "@core/net/node";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { VRouter } from "./getVRouters";

export class Client implements RpcClient<VRouter> {
    #requestManager: RequestManager<VRouter>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.CreateVRouter, localNodeService });
    }

    createRequest(destination: NodeId): Promise<[RpcRequest, Promise<RpcResult<VRouter>>]> {
        return this.#requestManager.createRequest(destination);
    }

    handleResponse(response: RpcResponse): void {
        if (response.status !== RpcStatus.Success) {
            this.#requestManager.resolveFailure(response.requestId, response.status);
            return;
        }

        const result = VRouter.deserialize(response.bodyReader);
        if (result.isErr()) {
            this.#requestManager.resolveFailure(response.requestId, RpcStatus.BadResponseFormat);
        } else {
            this.#requestManager.resolveSuccess(response.requestId, result.unwrap());
        }
    }
}