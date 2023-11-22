import { BufferWriter } from "@core/net/buffer";
import { SerializeU16, SerializeU8 } from "@core/serde";
import { RpcClient } from "../handler";
import { NodeId, ReactiveService } from "@core/net/routing";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";

class Params {
    mediaId: number;
    port: number;

    constructor(args: { mediaId: number; port: number }) {
        this.mediaId = args.mediaId;
        this.port = args.port;
    }

    serialize(writer: BufferWriter): void {
        new SerializeU8(this.mediaId).serialize(writer);
        new SerializeU16(this.port).serialize(writer);
    }

    serializedLength(): number {
        return 3;
    }
}

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ reactiveService }: { reactiveService: ReactiveService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.StartServer, reactiveService });
    }

    createRequest(destinationId: NodeId, port: number): [RpcRequest, Promise<RpcResult<void>>] {
        const body = new Params({ mediaId: 0, port });
        return this.#requestManager.createRequest(destinationId, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response.frameId);
    }
}
