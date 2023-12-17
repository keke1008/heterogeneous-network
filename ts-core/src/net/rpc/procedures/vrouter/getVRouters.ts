import { Destination, LocalNodeService } from "@core/net/node";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { DeserializeResult, DeserializeU16, DeserializeVector, SerializeVector } from "@core/serde";

export class VRouter {
    port: number;

    constructor({ port }: { port: number }) {
        this.port = port;
    }

    static deserialize(reader: BufferReader): DeserializeResult<VRouter> {
        return DeserializeU16.deserialize(reader).map((port) => new VRouter({ port }));
    }

    serialize(writer: BufferWriter): void {
        writer.writeUint16(this.port);
    }

    serializedLength(): number {
        return 2;
    }
}

export type VRouters = VRouter[];
export const VRouters = {
    deserialize: new DeserializeVector(VRouter).deserialize,
    serializable: (params: VRouters) => new SerializeVector(params),
};

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

        const result = VRouters.deserialize(response.bodyReader);
        if (result.isErr()) {
            this.#requestManager.resolveFailure(response.requestId, RpcStatus.BadResponseFormat);
        } else {
            this.#requestManager.resolveSuccess(response.requestId, result.unwrap());
        }
    }
}
