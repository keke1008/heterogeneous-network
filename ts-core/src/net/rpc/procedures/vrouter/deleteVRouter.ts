import { Destination } from "@core/net/node";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { LocalNodeService } from "@core/net/local";
import { ObjectSerdeable, Uint16Serdeable } from "@core/serde";

const paramSerdeable = new ObjectSerdeable({
    port: new Uint16Serdeable(),
});

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.DeleteVRouter, localNodeService });
    }

    createRequest(destination: Destination, opts: { port: number }): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        return this.#requestManager.createRequest(destination, paramSerdeable.serializer(opts));
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
