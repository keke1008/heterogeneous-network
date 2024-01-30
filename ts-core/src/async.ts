import { CancelListening, EventBroker } from "./event";
import { Duration } from "./time";

/**
 * @deprecated Use {@link Duration} instead.
 */
export const sleepMs = (ms: number): Promise<void> => {
    return sleep(Duration.fromMillies(ms));
};

/**
 * @deprecated Use {@link Duration} instead.
 */
export const sleepMsWithCancel = (ms: number): [Promise<void>, cancel: () => void] => {
    return sleepWithCancelable(Duration.fromMillies(ms));
};

/**
 * @deprecated Use {@link Duration} instead.
 */
export const withTimeoutMs = <T>(args: {
    timeoutMs: number;
    promise: Promise<T>;
    onTimeout?: () => T | Promise<T>;
}): Promise<T> => {
    return withTimeout({
        timeout: Duration.fromMillies(args.timeoutMs),
        promise: args.promise,
        onTimeout: args.onTimeout,
    });
};

export const sleep = (duration: Duration): Promise<void> => {
    return new Promise((resolve) => setTimeout(resolve, duration.millies));
};

export const sleepWithCancelable = (duration: Duration): [Promise<void>, cancel: () => void] => {
    let cancel: () => void;
    const promise = new Promise<void>((resolve) => {
        const id = setTimeout(resolve, duration.millies);
        cancel = () => clearTimeout(id);
    });
    return [promise, cancel!];
};

export const withTimeout = <T>(args: {
    timeout: Duration;
    promise: Promise<T>;
    onTimeout?: () => T | Promise<T>;
}): Promise<T> => {
    const { timeout, promise, onTimeout } = args;
    const [sleepPromise, cancel] = sleepWithCancelable(timeout);

    return Promise.race([
        promise.then((result) => {
            cancel();
            return result;
        }),
        sleepPromise.then(() => {
            if (onTimeout) {
                return onTimeout();
            } else {
                throw new Error("Timeout");
            }
        }),
    ]);
};

export const withCancel = <T, U = T>(args: {
    signal: AbortSignal;
    promise: Promise<T>;
    onCancel?: () => U | Promise<U>;
}): Promise<T | U> => {
    const { signal, promise, onCancel } = args;

    return Promise.race([
        promise,
        new Promise((resolve) => signal.addEventListener("abort", resolve)).then(() => {
            if (onCancel) {
                return onCancel();
            } else {
                throw new Error("Canceled");
            }
        }),
    ]);
};

export type Handle<T> = Promise<T> & {
    cancel(): void;
};

export interface Spawn<T> {
    (): Promise<T>;
    (callback: (signal: AbortSignal) => Promise<T>): Handle<T>;
}

export const spawn = <T>(callback: (signal: AbortSignal) => Promise<T>): Handle<T> => {
    const controller = new AbortController();
    const promise = callback(controller.signal);
    return Object.assign(promise, {
        cancel: () => controller.abort(),
    });
};

export class Delay {
    #duration: Duration;
    #cancel: () => void;
    #onTimeout = new EventBroker<void>();

    constructor(duration: Duration) {
        this.#duration = duration;
        const timeout = setTimeout(() => {
            this.#onTimeout.emit();
        }, this.#duration.millies);
        this.#cancel = () => clearTimeout(timeout);
    }

    reset(): void {
        this.#cancel();
        const timeout = setTimeout(() => {
            this.#onTimeout.emit();
        }, this.#duration.millies);
        this.#cancel = () => clearTimeout(timeout);
    }

    onTimeout(listener: () => void): CancelListening {
        return this.#onTimeout.listen(listener);
    }

    cancel(): void {
        this.#cancel();
    }
}

export const nextTick = (callback?: () => void): Promise<void> => {
    return Promise.resolve().then(callback);
};

export class Throttle<E> {
    #duration: Duration;

    #buffer: E[] = [];
    #onEmit = new EventBroker<E[]>();
    #timer?: Handle<void>;

    #spawnTimer() {
        if (this.#timer !== undefined) {
            throw new Error("Timer already spawned");
        }

        this.#timer = spawn(async () => {
            await sleep(this.#duration);

            const buffer = this.#buffer;
            this.#buffer = [];
            this.#onEmit.emit(buffer);

            this.#timer = undefined;
        });
    }

    constructor(duration: Duration) {
        this.#duration = duration;
        this.#onEmit.listenOnListenerAdded(() => {
            if (this.#buffer.length > 0 && this.#timer === undefined) {
                this.#spawnTimer();
            }
        });
    }

    emit(event: E): void {
        this.#buffer.push(event);
        if (this.#timer === undefined && this.#onEmit.hasListeners()) {
            this.#spawnTimer();
        }
    }

    onEmit(listener: (events: E[]) => void): CancelListening {
        return this.#onEmit.listen(listener);
    }
}
