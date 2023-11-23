export interface Deferred<T> extends Promise<T> {
    status: "pending" | "fulfilled" | "rejected";
    value?: T;
    resolve: (value: T) => void;
    reject: (reason?: unknown) => void;
}

export const deferred = <T>(): Deferred<T> => {
    const props: Partial<Deferred<T>> = { status: "pending" };
    const result: Partial<Deferred<T>> = new Promise<T>((resolve_, reject_) => {
        props.resolve = (value: T) => {
            if (result.status === "pending") {
                result.status = "fulfilled";
                result.value = value;
            }
            resolve_(value);
        };
        props.reject = (reason?: unknown) => {
            if (result.status === "pending") {
                result.status = "rejected";
            }
            reject_(reason);
        };
    });

    return Object.assign(result, props) as Deferred<T>;
};
