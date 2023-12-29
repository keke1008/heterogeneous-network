import { Destination } from "@core/net/node";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { ObjectSerdeable, SerdeableValue, Uint16Serdeable, VectorSerdeable } from "@core/serde";
import { LocalNodeService } from "@core/net/local";

export const VRouter = {
    serdeable: new ObjectSerdeable({
        port: new Uint16Serdeable(),
    }),
};
export type VRouter = SerdeableValue<typeof VRouter.serdeable>;

export const VRouters = {
    serdeable: new VectorSerdeable(VRouter.serdeable),
};
export type VRouters = SerdeableValue<typeof VRouters.serdeable>;

const resultSerdeable = new ObjectSerdeable({
    vRouters: new VectorSerdeable(VRouter.serdeable),
});

export class Client implements RpcClient<VRouter[]> {
    #requestManager: RequestManager<VRouter[]>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.GetVRouters, localNodeService });
    }

    createRequest(destination: Destination): Promise<[RpcRequest, Promise<RpcResult<VRouter[]>>]> {
        return this.#requestManager.createRequest(destination);
    }

    handleResponse(response: RpcResponse): void {
        if (response.status !== RpcStatus.Success) {
            this.#requestManager.resolveFailure(response.requestId, response.status);
            return;
        }

        const result = resultSerdeable.deserializer().deserialize(response.bodyReader);
        if (result.isErr()) {
            this.#requestManager.resolveFailure(response.requestId, RpcStatus.BadResponseFormat);
        } else {
            this.#requestManager.resolveSuccess(response.requestId, result.unwrap().vRouters);
        }
    }
}
