export const sleepMs = (ms: number): Promise<void> => new Promise((resolve) => setTimeout(resolve, ms));

export const sleepMsWithCancel = (ms: number): [Promise<void>, cancel: () => void] => {
    let cancel: () => void;
    const promise = new Promise<void>((resolve) => {
        const id = setTimeout(resolve, ms);
        cancel = () => clearTimeout(id);
    });
    return [promise, cancel!];
};

export const withTimeoutMs = <T>(args: {
    timeoutMs: number;
    promise: Promise<T>;
    onTimeout?: () => T | Promise<T>;
}): Promise<T> => {
    const { timeoutMs, promise, onTimeout } = args;
    const [sleepPromise, cancel] = sleepMsWithCancel(timeoutMs);

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
