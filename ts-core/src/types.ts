export type UnionToIntersection<U> = (U extends unknown ? (k: U) => void : never) extends (k: infer I) => void
    ? I
    : never;

export const unreachable = (_: never): never => {
    throw new Error("unreachable");
};
