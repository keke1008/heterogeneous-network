import { Destination } from "@core/net/node";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { SerializeU16 } from "@core/serde";
import { LocalNodeService } from "@core/net/local";

const Params = SerializeU16;

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.DeleteVRouter, localNodeService });
    }

    createRequest(destination: Destination, opts: { port: number }): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        return this.#requestManager.createRequest(destination, new Params(opts.port));
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
