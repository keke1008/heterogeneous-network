import { FixedBytesSerdeable, ObjectSerdeable, SerdeableValue } from "@core/serde";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { MediaPortNumber } from "@core/net/link";
import { Destination } from "@core/net/node";
import { LocalNodeService } from "@core/net/local";

const paramSerdeable = new ObjectSerdeable({
    portNumber: MediaPortNumber.serdeable,
    mask: new FixedBytesSerdeable(4),
});

export type SetEthernetSubnetMaskParam = SerdeableValue<typeof paramSerdeable>;

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.SetEthernetSubnetMask, localNodeService });
    }

    createRequest(
        destination: Destination,
        args: SetEthernetSubnetMaskParam,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = paramSerdeable.serializer(args);
        return this.#requestManager.createRequest(destination, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
