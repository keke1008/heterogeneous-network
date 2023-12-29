import { MediaPortNumber, SerialAddress } from "@core/net/link";
import { RpcClient } from "../handler";
import { RequestManager, RpcResult } from "../../request";
import { Destination } from "@core/net/node";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { LocalNodeService } from "@core/net/local";
import { ObjectSerdeable } from "@core/serde";

const paramSerdeable = new ObjectSerdeable({
    portNumber: MediaPortNumber.serdeable,
    address: SerialAddress.serdeable,
});

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.SetAddress, localNodeService });
    }

    createRequest(
        destination: Destination,
        portNumber: MediaPortNumber,
        address: SerialAddress,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = paramSerdeable.serializer({ portNumber, address });
        return this.#requestManager.createRequest(destination, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
