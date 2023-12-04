import { ObjectMap } from "@core/object";
import { FrameType, Procedure, RpcRequest, RpcStatus } from "./frame";
import { withTimeoutMs } from "@core/async";
import { NodeId } from "@core/net/node";
import { ReactiveService } from "../routing";
import { Serializable } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { IncrementalRequestIdGenerator, RequestId } from "./requestId";

export type RpcResult<T> = { status: RpcStatus.Success; value: T } | { status: Exclude<RpcStatus, RpcStatus.Success> };

type Resolve<T> = (result: RpcResult<T>) => void;

class RequestTimeKeeper<T> {
    #resolves: ObjectMap<RequestId, Resolve<T>, number> = new ObjectMap((id) => id.get());
    #frameIdGenerator = new IncrementalRequestIdGenerator();
    #timeoutMs: number;

    constructor(opts?: { timeoutMs?: number }) {
        this.#timeoutMs = opts?.timeoutMs ?? 5000;
    }

    createRequest(): [RequestId, Promise<RpcResult<T>>] {
        const frameId = this.#frameIdGenerator.generate();
        const promise = withTimeoutMs<RpcResult<T>>({
            timeoutMs: this.#timeoutMs,
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
    #reactiveService: ReactiveService;
    #procedure: Procedure;

    constructor(args: { reactiveService: ReactiveService; procedure: Procedure }) {
        this.#reactiveService = args.reactiveService;
        this.#procedure = args.procedure;
    }

    async createRequest(destinationId: NodeId, body?: Serializable): Promise<[RpcRequest, Promise<RpcResult<T>>]> {
        const writer = new BufferWriter(new Uint8Array(body?.serializedLength() ?? 0));
        body?.serialize(writer);

        const [requestId, promise] = this.#timeKeeper.createRequest();
        const request: RpcRequest = {
            frameType: FrameType.Request,
            procedure: this.#procedure,
            requestId,
            senderId: await this.#reactiveService.localNodeInfo().getId(),
            targetId: destinationId,
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

    resolveVoid(this: RequestManager<void>, frameId: RequestId): void {
        this.#timeKeeper.resolve(frameId, { status: RpcStatus.Success, value: undefined });
    }
}
