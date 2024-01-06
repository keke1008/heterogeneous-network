import { deferred, Deferred } from "./deferred";

class Entry<T> {
    readonly #next: Deferred<IteratorResult<[T, Entry<T>], void>>;
    #done: boolean = false;

    constructor(next?: Deferred<IteratorResult<[T, Entry<T>], void>>) {
        this.#next = next ?? deferred();
    }

    #assertNotResolved(): void {
        if (this.#next.status !== "pending") {
            throw new Error("Entry is already resolved");
        }
    }

    isDone(): boolean {
        return this.#done;
    }

    resolve(value: T): Entry<T> {
        this.#assertNotResolved();

        const next = new Entry<T>();
        this.#next.resolve({ done: false, value: [value, next] });
        return next;
    }

    resolveDone(): void {
        this.#assertNotResolved();

        this.#done = true;
        this.#next.resolve({ done: true, value: undefined });
    }

    async getValue(): Promise<IteratorResult<T, void>> {
        const next = await this.#next;
        return next.done ? next : { done: false, value: next.value[0] };
    }

    async nextEntry(): Promise<Entry<T> | undefined> {
        const next = await this.#next;
        return next.done ? undefined : next.value[1];
    }

    tryNextEntry(): Entry<T> | undefined {
        const value = this.#next.value?.value ?? undefined;
        return value?.[1];
    }
}

class Close {
    #controller: AbortController = new AbortController();

    close(): void {
        this.#controller.abort();
    }

    onClose(f: () => void): void {
        this.#controller.signal.addEventListener("abort", f);
    }

    isClosed(): boolean {
        return this.#controller.signal.aborted;
    }
}

export interface ISender<T> {
    send(value: T): void;
    close(): void;
    onClose(f: () => void): void;
    isClosed(): boolean;
}

export class AbstractSender<T> implements ISender<T> {
    #sender: ISender<T>;

    constructor(sender: ISender<T>) {
        this.#sender = sender;
    }

    send(value: T): void {
        this.#sender.send(value);
    }

    close(): void {
        this.#sender.close();
    }

    onClose(f: () => void): void {
        this.#sender.onClose(f);
    }

    isClosed(): boolean {
        return this.#sender.isClosed();
    }
}

export class Sender<T> implements ISender<T> {
    #head: Entry<T>;
    #close: Close;

    constructor(head?: Entry<T>, close?: Close) {
        this.#head = head ?? new Entry(deferred());
        this.#close = close ?? new Close();
    }

    #updateHead(): void {
        let next = this.#head.tryNextEntry();
        while (next !== undefined) {
            this.#head = next;
            next = this.#head.tryNextEntry();
        }
    }

    receiver(): Receiver<T> {
        this.#updateHead();
        return new Receiver(this.#head, this.#close);
    }

    send(value: T): void {
        if (this.isClosed()) {
            throw new Error("Sender is closed");
        }
        this.#updateHead();
        this.#head = this.#head.resolve(value);
    }

    close(): void {
        this.#updateHead();
        this.#head.resolveDone();
        this.#close.close();
    }

    onClose(f: () => void): void {
        return this.#close.onClose(f);
    }

    isClosed(): boolean {
        return this.#close.isClosed();
    }
}

export interface IReceiver<T> {
    [Symbol.asyncIterator](): AsyncIterator<T, void, AbortSignal | undefined>;

    next(abort?: AbortSignal): Promise<IteratorResult<T, void>>;
    close(): void;
    isClosed(): boolean;
    onClose(f: () => void): void;
    map<U>(callback: (value: T) => U): IReceiver<U>;
    filter(callback: (value: T) => boolean): IReceiver<T>;
    filterMap<U>(callback: (value: T) => U | undefined): IReceiver<U>;
    tap(callback: (value: T) => void): IReceiver<T>;
    forEach(callback: (value: T) => void, abort?: AbortSignal): Promise<void>;
    pipeTo(sender: ISender<T>): Promise<void>;
}

export abstract class AbstractReceiver<T> implements AsyncIterable<T>, IReceiver<T> {
    abstract next(abort?: AbortSignal): Promise<IteratorResult<T, void>>;
    abstract close(): void;
    abstract isClosed(): boolean;
    abstract onClose(f: () => void): void;

    [Symbol.asyncIterator]() {
        return this;
    }

    map<U>(callback: (value: T) => U): IReceiver<U> {
        return new Map(this, callback);
    }

    filter(callback: (value: T) => boolean): IReceiver<T> {
        return new Filter(this, callback);
    }

    filterMap<U>(callback: (value: T) => U | undefined): IReceiver<U> {
        return new FilterMap(this, callback);
    }

    tap(callback: (value: T) => void): IReceiver<T> {
        return new Tap(this, callback);
    }

    async forEach(callback: (value: T) => void, abort?: AbortSignal): Promise<void> {
        let next = await this.next(abort);
        while (!next.done) {
            callback(next.value);
            next = await this.next(abort);
        }
    }

    async pipeTo(sender: ISender<T>): Promise<void> {
        await this.forEach((value) => sender.send(value));
    }
}

export class Receiver<T> extends AbstractReceiver<T> {
    #head: Entry<T>;
    #close: Close;

    constructor(head: Entry<T>, close: Close) {
        super();
        this.#head = head;
        this.#close = close;
    }

    override async next(abort?: AbortSignal): Promise<IteratorResult<T, void>> {
        const value = await this.#head.getValue();
        const entry = await this.#head.nextEntry();

        if (abort?.aborted) {
            return new Promise<never>(() => {});
        }

        if (entry !== undefined) {
            this.#head = entry;
        }
        return value;
    }

    override close(): void {
        this.#close.close();
    }

    override isClosed(): boolean {
        return this.#head.isDone();
    }

    override onClose(f: () => void): void {
        this.#close.onClose(f);
    }
}

export class Map<T, U> extends AbstractReceiver<U> {
    #receiver: IReceiver<T>;
    #f: (value: T) => U;

    constructor(receiver: IReceiver<T>, callback: (value: T) => U) {
        super();
        this.#receiver = receiver;
        this.#f = callback;
    }

    override async next(abort: AbortSignal): Promise<IteratorResult<U, void>> {
        const next = await this.#receiver.next(abort);
        return next.done ? next : { done: false, value: this.#f(next.value) };
    }

    override close(): void {
        this.#receiver.close();
    }

    override isClosed(): boolean {
        return this.#receiver.isClosed();
    }

    override onClose(f: () => void): void {
        this.#receiver.onClose(f);
    }
}

export class Filter<T> extends AbstractReceiver<T> {
    #receiver: IReceiver<T>;
    #f: (value: T) => boolean;

    constructor(receiver: IReceiver<T>, callback: (value: T) => boolean) {
        super();
        this.#receiver = receiver;
        this.#f = callback;
    }

    override async next(abort: AbortSignal): Promise<IteratorResult<T, void>> {
        const value = await this.#receiver.next(abort);
        return value.done || this.#f(value.value) ? value : this.next(abort);
    }

    override close(): void {
        this.#receiver.close();
    }

    override isClosed(): boolean {
        return this.#receiver.isClosed();
    }

    override onClose(f: () => void): void {
        this.#receiver.onClose(f);
    }
}

export class FilterMap<T, U> extends AbstractReceiver<U> {
    #receiver: IReceiver<T>;
    #f: (value: T) => U | undefined;

    constructor(receiver: IReceiver<T>, callback: (value: T) => U | undefined) {
        super();
        this.#receiver = receiver;
        this.#f = callback;
    }

    override async next(abort: AbortSignal): Promise<IteratorResult<U, void>> {
        const value = await this.#receiver.next();
        if (value.done) {
            return value;
        }

        const result = this.#f(value.value);
        if (result === undefined) {
            return this.next(abort);
        }

        return { done: false, value: result };
    }

    override close(): void {
        this.#receiver.close();
    }

    override isClosed(): boolean {
        return this.#receiver.isClosed();
    }

    override onClose(f: () => void): void {
        this.#receiver.onClose(f);
    }
}

export class Tap<T> extends AbstractReceiver<T> {
    #receiver: IReceiver<T>;
    #f: (value: T) => void;

    constructor(receiver: IReceiver<T>, callback: (value: T) => void) {
        super();
        this.#receiver = receiver;
        this.#f = callback;
    }

    override async next(abort: AbortSignal): Promise<IteratorResult<T, void>> {
        const value = await this.#receiver.next(abort);
        if (!value.done) {
            this.#f(value.value);
        }
        return value;
    }

    override close(): void {
        this.#receiver.close();
    }

    override isClosed(): boolean {
        return this.#receiver.isClosed();
    }

    override onClose(f: () => void): void {
        this.#receiver.onClose(f);
    }
}
