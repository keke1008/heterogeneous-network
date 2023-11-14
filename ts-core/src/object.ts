export class ObjectMap<T, V, K> {
    #key: (value: T) => K;
    #keyMap: Map<K, T> = new Map();
    #valueMap: Map<K, V> = new Map();

    constructor(key: (value: T) => K) {
        this.#key = key;
    }

    clear() {
        this.#keyMap.clear();
        this.#valueMap.clear();
    }

    delete(key: T): boolean {
        const k = this.#key(key);
        this.#keyMap.delete(k);
        return this.#valueMap.delete(k);
    }

    entries(): IterableIterator<[T, V]> {
        const keyEntries = this.#keyMap.entries();
        return {
            next: () => {
                const next = keyEntries.next();
                if (next.done) {
                    return next;
                }
                const [k, t] = next.value;
                return { value: [t, this.#valueMap.get(k)!], done: false };
            },
            [Symbol.iterator]: () => this.entries(),
        };
    }

    forEach(callbackfn: (value: V, key: T, map: ObjectMap<T, V, K>) => void) {
        for (const [k, v] of this.entries()) {
            callbackfn(v, k, this);
        }
    }

    get(key: T): V | undefined {
        return this.#valueMap.get(this.#key(key));
    }

    has(key: T): boolean {
        return this.#keyMap.has(this.#key(key));
    }

    keys(): IterableIterator<T> {
        return this.#keyMap.values();
    }

    set(key: T, value: V): this {
        const k = this.#key(key);
        this.#keyMap.set(k, key);
        this.#valueMap.set(k, value);
        return this;
    }

    values(): IterableIterator<V> {
        return this.#valueMap.values();
    }
}

export class ObjectSet<T, K> {
    #key: (value: T) => K;
    #keyMap: Map<K, T> = new Map();

    constructor(key: (value: T) => K) {
        this.#key = key;
    }

    add(value: T): this {
        this.#keyMap.set(this.#key(value), value);
        return this;
    }

    clear() {
        this.#keyMap.clear();
    }

    delete(value: T): boolean {
        return this.#keyMap.delete(this.#key(value));
    }

    entries(): IterableIterator<[T, T]> {
        const keyEntries = this.#keyMap.entries();
        return {
            next: () => {
                const next = keyEntries.next();
                if (next.done) {
                    return next;
                }
                const [_, t] = next.value;
                return { value: [t, t], done: false };
            },
            [Symbol.iterator]: () => this.entries(),
        };
    }

    forEach(callbackfn: (value: T, key: T, map: ObjectSet<T, K>) => void) {
        for (const [k, v] of this.entries()) {
            callbackfn(v, k, this);
        }
    }

    get(value: T): T | undefined {
        return this.#keyMap.get(this.#key(value));
    }

    has(value: T): boolean {
        return this.#keyMap.has(this.#key(value));
    }

    keys(): IterableIterator<T> {
        return this.#keyMap.values();
    }

    values(): IterableIterator<T> {
        return this.#keyMap.values();
    }

    [Symbol.iterator](): IterableIterator<T> {
        return this.#keyMap.values();
    }
}
