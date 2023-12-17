import { Destination, LocalNodeService } from "@core/net/node";
import { Procedure, RpcRequest, RpcResponse } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { RpcClient } from "../handler";
import { BufferWriter } from "@core/net/buffer";

export enum BlinkOperation {
    Blink = 1,
    Stop = 2,
}

class Params {
    operation: BlinkOperation;

    constructor(args: { operation: BlinkOperation }) {
        this.operation = args.operation;
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.operation);
    }

    serializedLength(): number {
        return 1;
    }
}

export class Client implements RpcClient<void> {
    #requestManager: RequestManager<void>;

    constructor({ localNodeService }: { localNodeService: LocalNodeService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.Blink, localNodeService });
    }

    createRequest(
        destination: Destination,
        operation: BlinkOperation,
    ): Promise<[RpcRequest, Promise<RpcResult<void>>]> {
        const body = new Params({ operation });
        return this.#requestManager.createRequest(destination, body);
    }

    handleResponse(response: RpcResponse): void {
        this.#requestManager.resolveVoid(response);
    }
}
