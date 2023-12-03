export type UnionToIntersection<U> = (U extends unknown ? (k: U) => void : never) extends (k: infer I) => void
    ? I
    : never;

// eslint-disable-next-line @typescript-eslint/no-unused-vars
export const consume = <T>(_: T): void => {};

export const unreachable = (_: never): never => {
    consume(_);
    throw new Error("unreachable");
};
