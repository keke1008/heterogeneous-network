import { BufferWriter } from "@core/net/buffer";
import { Destination } from "@core/net/node";
import { SerializeU16 } from "@core/serde";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { MediaPortNumber } from "@core/net/link";
import { LocalNodeService } from "@core/net/local";

class Params {
    mediaNumber: MediaPortNumber;
    port: number;

    constructor(args: { mediaNumber: MediaPortNumber; port: number }) {
        this.mediaNumber = args.mediaNumber;
        this.port = args.port;
    }

    serialize(writer: BufferWriter): void {
        this.mediaNumber.serialize(writer);
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

    createRequest(
        destination: Destination,
        port: number,
        mediaNumber: MediaPortNumber,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = new Params({ mediaNumber, port });
        return this.#requestManager.createRequest(destination, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
