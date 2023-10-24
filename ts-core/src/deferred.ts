export interface Deferred<T> {
    status: "pending" | "fulfilled" | "rejected";
    promise: Promise<T>;
    resolve: (value: T) => void;
    reject: (reason?: unknown) => void;
}

export const deferred = <T>(): Deferred<T> => {
    const result: Partial<Deferred<T>> = { status: "pending" };
    result.promise = new Promise<T>((resolve_, reject_) => {
        result.resolve = (value: T) => {
            if (result.status === "pending") {
                result.status = "fulfilled";
            }
            resolve_(value);
        };
        result.reject = (reason?: unknown) => {
            if (result.status === "pending") {
                result.status = "rejected";
            }
            reject_(reason);
        };
    });
    return result as Deferred<T>;
};
