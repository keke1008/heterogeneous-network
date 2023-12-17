import { BufferWriter } from "@core/net/buffer";
import { Destination, LocalNodeService } from "@core/net/node";
import { SerializeBytes, SerializeU8 } from "@core/serde";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";

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
        return (
            new SerializeU8(this.mediaId).serializedLength() +
            new SerializeBytes(this.ssid).serializedLength() +
            new SerializeBytes(this.password).serializedLength()
        );
    }
}

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.ConnectToAccessPoint, localNodeService });
    }

    createRequest(
        destination: Destination,
        ssid: Uint8Array,
        password: Uint8Array,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = new Params({ mediaId: 0, ssid, password });
        return this.#requestManager.createRequest(destination, body);
    }
    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
