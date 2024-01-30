import { LocalNodeService } from "@core/net/local";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RpcClient } from "../handler";
import { Destination } from "@core/net/node";
import { RequestManager, RpcResult } from "../../request";
import { Config, configSerdeable } from "./config";

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.SetConfig, localNodeService });
    }

    createRequest(destination: Destination, config: 0 | Config): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        return this.#requestManager.createRequest(destination, configSerdeable.serializer(config as Config));
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
