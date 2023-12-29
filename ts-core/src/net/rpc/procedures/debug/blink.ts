import { Destination } from "@core/net/node";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { LocalNodeService } from "@core/net/local";
import { EnumSerdeable } from "@core/serde";

export enum BlinkOperation {
    Blink = 1,
    Stop = 2,
}

const paramSerdeable = new EnumSerdeable<BlinkOperation>(BlinkOperation);

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.Blink, localNodeService });
    }

    createRequest(
        destination: Destination,
        operation: BlinkOperation,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        return this.#requestManager.createRequest(destination, paramSerdeable.serializer(operation));
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
