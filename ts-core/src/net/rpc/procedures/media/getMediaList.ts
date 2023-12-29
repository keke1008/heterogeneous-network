import { Address, AddressType } from "@core/net/link";
import { RpcClient } from "../handler";
import { Destination } from "@core/net/node";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { EnumSerdeable, ObjectSerdeable, OptionalSerdeable, SerdeableValue, VectorSerdeable } from "@core/serde";
import { LocalNodeService } from "@core/net/local";

const mediaInfoSerdeable = new ObjectSerdeable({
    addressType: new OptionalSerdeable(new EnumSerdeable<AddressType>(AddressType)),
    address: new OptionalSerdeable(Address.serdeable),
});

export type MediaInfo = SerdeableValue<typeof mediaInfoSerdeable>;

const paramSerdeable = new VectorSerdeable(mediaInfoSerdeable);

export class Client implements RpcClient<MediaInfo[]> {
    #requestManager: RequestManager<MediaInfo[]>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.GetMediaList, localNodeService });
    }

    createRequest(destination: Destination): Promise<[RpcRequest, Promise<RpcResult<MediaInfo[]>>]> {
        return this.#requestManager.createRequest(destination);
    }

    handleResponse(response: RpcResponse): void {
        const list = paramSerdeable.deserializer().deserialize(response.bodyReader);
        if (list.isOk()) {
            this.#requestManager.resolveSuccess(response.requestId, list.unwrap());
        } else {
            this.#requestManager.resolveFailure(response.requestId, RpcStatus.Failed);
        }
    }
}
