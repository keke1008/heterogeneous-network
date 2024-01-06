export const createDummy = <T extends object>(name: string): T => {
    return new Proxy<T>({} as T, {
        get() {
            throw new Error(`Cannot access property ${name} of dummy object`);
        },
    });
};
