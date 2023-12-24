import { BufferWriter } from "@core/net/buffer";
import { MediaPortNumber, SerialAddress } from "@core/net/link";
import { RpcClient } from "../handler";
import { RequestManager, RpcResult } from "../../request";
import { Destination } from "@core/net/node";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { LocalNodeService } from "@core/net/local";

class Params {
    portNumber: MediaPortNumber;
    address: SerialAddress;

    constructor(args: { portNumber: MediaPortNumber; address: SerialAddress }) {
        this.portNumber = args.portNumber;
        this.address = args.address;
    }

    serialize(writer: BufferWriter) {
        this.portNumber.serialize(writer);
        this.address.serialize(writer);
    }

    serializedLength(): number {
        return this.portNumber.serializedLength() + this.address.serializedLength();
    }
}

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.SetAddress, localNodeService });
    }

    createRequest(
        destination: Destination,
        portNumber: MediaPortNumber,
        address: SerialAddress,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = new Params({ portNumber, address });
        return this.#requestManager.createRequest(destination, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
