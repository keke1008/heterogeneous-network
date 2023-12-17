import { BufferWriter } from "@core/net/buffer";
import { Destination, LocalNodeService } from "@core/net/node";
import { SerializeU16, SerializeU8 } from "@core/serde";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";

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

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.StartServer, localNodeService });
    }

    createRequest(destination: Destination, port: number): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = new Params({ mediaId: 0, port });
        return this.#requestManager.createRequest(destination, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
