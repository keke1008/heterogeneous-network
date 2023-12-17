import { ObjectMap } from "@core/object";
import { FrameType, Procedure, RpcRequest, RpcResponse, RpcStatus } from "./frame";
import { withTimeout } from "@core/async";
import { LocalNodeService } from "@core/net/node";
import { Serializable } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { IncrementalRequestIdGenerator, RequestId } from "./requestId";
import { Destination } from "../discovery";
import { Duration } from "@core/time";

export type RpcResult<T> = { status: RpcStatus.Success; value: T } | { status: Exclude<RpcStatus, RpcStatus.Success> };

type Resolve<T> = (result: RpcResult<T>) => void;

class RequestTimeKeeper<T> {
    #resolves: ObjectMap<RequestId, Resolve<T>, number> = new ObjectMap((id) => id.get());
    #frameIdGenerator = new IncrementalRequestIdGenerator();
    #timeout: Duration;

    constructor(opts?: { timeout?: Duration }) {
        this.#timeout = opts?.timeout ?? Duration.fromSeconds(5);
    }

    createRequest(): [RequestId, Promise<RpcResult<T>>] {
        const frameId = this.#frameIdGenerator.generate();
        const promise = withTimeout<RpcResult<T>>({
            timeout: this.#timeout,
            promise: new Promise<RpcResult<T>>((resolve) => this.#resolves.set(frameId, resolve)),
            onTimeout: () => ({ status: RpcStatus.Timeout }),
        }).finally(() => this.#resolves.delete(frameId));

        return [frameId, promise];
    }

    resolve(frameId: RequestId, result: RpcResult<T>): void {
        this.#resolves.get(frameId)?.(result);
    }
}

export class RequestManager<T> {
    #timeKeeper: RequestTimeKeeper<T> = new RequestTimeKeeper();
    #localNodeService: LocalNodeService;
    #procedure: Procedure;

    constructor(args: { localNodeService: LocalNodeService; procedure: Procedure }) {
        this.#localNodeService = args.localNodeService;
        this.#procedure = args.procedure;
    }

    async createRequest(destination: Destination, body?: Serializable): Promise<[RpcRequest, Promise<RpcResult<T>>]> {
        const writer = new BufferWriter(new Uint8Array(body?.serializedLength() ?? 0));
        body?.serialize(writer);

        const [requestId, promise] = this.#timeKeeper.createRequest();
        const request: RpcRequest = {
            frameType: FrameType.Request,
            procedure: this.#procedure,
            requestId,
            clientId: await this.#localNodeService.getId(),
            server: destination,
            bodyReader: new BufferReader(writer.unwrapBuffer()),
        };

        return [request, promise];
    }

    resolve(frameId: RequestId, result: RpcResult<T>): void {
        this.#timeKeeper.resolve(frameId, result);
    }

    resolveSuccess(frameId: RequestId, value: T): void {
        this.#timeKeeper.resolve(frameId, { status: RpcStatus.Success, value });
    }

    resolveFailure(frameId: RequestId, status: Exclude<RpcStatus, RpcStatus.Success>): void {
        this.#timeKeeper.resolve(frameId, { status });
    }

    resolveVoid(this: RequestManager<void>, response: RpcResponse): void {
        this.#timeKeeper.resolve(response.requestId, { status: response.status, value: undefined });
    }
}
