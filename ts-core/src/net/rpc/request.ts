import { ObjectMap } from "@core/object";
import { RpcStatus } from "./frame";
import { withTimeoutMs } from "@core/async";
import { FrameId, IncrementalFrameIdGenerator } from "../link";

export type RpcResult<T> = { status: RpcStatus.Success; value: T } | { status: Exclude<RpcStatus, RpcStatus.Success> };

type Resolve<T> = (result: RpcResult<T>) => void;

export class RequestManager<T> {
    #resolves: ObjectMap<FrameId, Resolve<T>, number> = new ObjectMap((id) => id.get());
    #frameIdGenerator = new IncrementalFrameIdGenerator();
    #timeoutMs: number;

    constructor(opts?: { timeoutMs?: number }) {
        this.#timeoutMs = opts?.timeoutMs ?? 5000;
    }

    request(): [FrameId, Promise<RpcResult<T>>] {
        const frameId = this.#frameIdGenerator.generate();
        const promise = withTimeoutMs<RpcResult<T>>({
            timeoutMs: this.#timeoutMs,
            promise: new Promise<RpcResult<T>>((resolve) => this.#resolves.set(frameId, resolve)),
            onTimeout: () => ({ status: RpcStatus.Timeout }),
        }).finally(() => this.#resolves.delete(frameId));

        return [frameId, promise];
    }

    resolveSuccess(frameId: FrameId, value: T): void {
        this.#resolves.get(frameId)?.({ status: RpcStatus.Success, value });
    }

    resolveFailure(frameId: FrameId, status: Exclude<RpcStatus, RpcStatus.Success>): void {
        this.#resolves.get(frameId)?.({ status });
    }

    resolve(frameId: FrameId, result: RpcResult<T>): void {
        this.#resolves.get(frameId)?.(result);
    }

    resolveVoid(this: RequestManager<void>, frameId: FrameId, status: RpcStatus): void {
        this.#resolves.get(frameId)?.({ status } as RpcResult<void>);
    }
}
