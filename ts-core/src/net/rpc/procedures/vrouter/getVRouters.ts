import { LocalNodeService, NodeId } from "@core/net/node";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { DeserializeResult, DeserializeU16, DeserializeVector } from "@core/serde";

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
}

const deserializeResponse = new DeserializeVector(VRouter);

export class Client implements RpcClient<VRouter[]> {
    #requestManager: RequestManager<VRouter[]>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.GetVRouters, localNodeService });
    }

    createRequest(destination: NodeId): Promise<[RpcRequest, Promise<RpcResult<VRouter[]>>]> {
        return this.#requestManager.createRequest(destination);
    }

    handleResponse(response: RpcResponse): void {
        if (response.status !== RpcStatus.Success) {
            this.#requestManager.resolveFailure(response.requestId, response.status);
            return;
        }

        const result = deserializeResponse.deserialize(response.bodyReader);
        if (result.isErr()) {
            this.#requestManager.resolveFailure(response.requestId, RpcStatus.BadResponseFormat);
        } else {
            this.#requestManager.resolveSuccess(response.requestId, result.unwrap());
        }
    }
}
