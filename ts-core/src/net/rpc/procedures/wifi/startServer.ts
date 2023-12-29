import { Destination } from "@core/net/node";
import { ObjectSerdeable, Uint16Serdeable } from "@core/serde";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { MediaPortNumber } from "@core/net/link";
import { LocalNodeService } from "@core/net/local";

const paramSerdeable = new ObjectSerdeable({
    mediaNumber: MediaPortNumber.serdeable,
    port: new Uint16Serdeable(),
});

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.StartServer, localNodeService });
    }

    createRequest(
        destination: Destination,
        port: number,
        mediaNumber: MediaPortNumber,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = paramSerdeable.serializer({ mediaNumber, port });
        return this.#requestManager.createRequest(destination, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
