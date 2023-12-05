import { NodeId } from "@core/net/node";
import { ReactiveService } from "@core/net/routing";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { BufferWriter } from "@core/net/buffer";
import { SerializeBytes, SerializeU8 } from "@core/serde";

class Params {
    mediaId: number;
    ssid: Uint8Array;
    password: Uint8Array;

    constructor(args: { mediaId: number; ssid: Uint8Array; password: Uint8Array }) {
        this.mediaId = args.mediaId;
        this.ssid = args.ssid;
        this.password = args.password;
    }

    serialize(writer: BufferWriter): void {
        new SerializeU8(this.mediaId).serialize(writer);
        new SerializeBytes(this.ssid).serialize(writer);
        new SerializeBytes(this.password).serialize(writer);
    }

    serializedLength(): number {
        return 1 + this.ssid.length + this.password.length;
    }
}

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ reactiveService }: { reactiveService: ReactiveService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.ConnectToAccessPoint, reactiveService });
    }

    createRequest(
        destinationId: NodeId,
        ssid: Uint8Array,
        password: Uint8Array,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = new Params({ mediaId: 0, ssid, password });
        return this.#requestManager.createRequest(destinationId, body);
    }
    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
