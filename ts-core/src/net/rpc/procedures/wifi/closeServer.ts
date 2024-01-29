import { Destination } from "@core/net/node";
import { ObjectSerdeable } from "@core/serde";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { MediaPortNumber } from "@core/net/link";
import { LocalNodeService } from "@core/net/local";

const paramSerdeable = new ObjectSerdeable({
    mediaNumber: MediaPortNumber.serdeable,
});

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.CloseServer, localNodeService });
    }

    createRequest(
        destination: Destination,
        mediaNumber: MediaPortNumber,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = paramSerdeable.serializer({ mediaNumber });
        return this.#requestManager.createRequest(destination, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
