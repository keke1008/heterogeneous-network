import { Destination } from "@core/net/node";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { Config, configSerdeable } from "./config";
import { LocalNodeService } from "@core/net/local";

export class Client implements RpcClient<Config> {
    #requestManager: RequestManager<Config>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.GetConfig, localNodeService });
    }

    createRequest(destination: Destination): Promise<[RpcRequest, Promise<RpcResult<Config>>]> {
        return this.#requestManager.createRequest(destination);
    }

    handleResponse(response: RpcResponse): void {
        if (response.status !== RpcStatus.Success) {
            this.#requestManager.resolveFailure(response.requestId, response.status);
            return;
        }

        const config = configSerdeable.deserializer().deserialize(response.bodyReader);
        if (config.isOk()) {
            this.#requestManager.resolveSuccess(response.requestId, config.unwrap());
        } else {
            this.#requestManager.resolveFailure(response.requestId, RpcStatus.Failed);
        }
    }
}
