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
