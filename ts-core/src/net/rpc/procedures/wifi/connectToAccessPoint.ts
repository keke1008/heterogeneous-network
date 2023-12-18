import { BufferWriter } from "@core/net/buffer";
import { Destination, LocalNodeService } from "@core/net/node";
import { SerializeBytes } from "@core/serde";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { MediaPortNumber } from "@core/net/link";

class Params {
    mediaNumber: MediaPortNumber;
    ssid: Uint8Array;
    password: Uint8Array;

    constructor(args: { mediaNumber: MediaPortNumber; ssid: Uint8Array; password: Uint8Array }) {
        this.mediaNumber = args.mediaNumber;
        this.ssid = args.ssid;
        this.password = args.password;
    }

    serialize(writer: BufferWriter): void {
        this.mediaNumber.serialize(writer);
        new SerializeBytes(this.ssid).serialize(writer);
        new SerializeBytes(this.password).serialize(writer);
    }

    serializedLength(): number {
        return (
            this.mediaNumber.serializedLength() +
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
        mediaNumber: MediaPortNumber
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = new Params({ mediaNumber, ssid, password });
        return this.#requestManager.createRequest(destination, body);
    }
    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
