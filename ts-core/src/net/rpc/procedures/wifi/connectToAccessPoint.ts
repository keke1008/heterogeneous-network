import { Destination } from "@core/net/node";
import { ObjectSerdeable, Uint8Serdeable, VariableBytesSerdeable } from "@core/serde";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { MediaPortNumber } from "@core/net/link";
import { LocalNodeService } from "@core/net/local";

const paramSerdeable = new ObjectSerdeable({
    mediaNumber: MediaPortNumber.serdeable,
    ssid: new VariableBytesSerdeable(new Uint8Serdeable()),
    password: new VariableBytesSerdeable(new Uint8Serdeable()),
});

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.ConnectToAccessPoint, localNodeService });
    }

    createRequest(
        destination: Destination,
        ssid: Uint8Array,
        password: Uint8Array,
        mediaNumber: MediaPortNumber,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = paramSerdeable.serializer({ mediaNumber, ssid, password });
        return this.#requestManager.createRequest(destination, body);
    }
    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
