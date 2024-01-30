type Queue<T> = T[];
type Handler<T> = (event: T) => void;

/**
 * @deprecated use EventBroker instead
 */
export class EventDispatcher<E> {
    #handler: Queue<E> | Handler<E> = [];

    emit(event: E): void {
        if (typeof this.#handler === "function") {
            this.#handler(event);
        } else {
            this.#handler.push(event);
        }
    }

    setHandler(handler: (event: E) => void): () => void {
        if (typeof this.#handler === "function") {
            throw new Error("handler is already set");
        }
        const queue = this.#handler;
        this.#handler = handler;
        for (const event of queue) {
            handler(event);
        }

        return () => {
            if (this.#handler === handler) {
                this.#handler = [];
            }
        };
    }
}

export interface CancelListening {
    (): void;
}

class CoreBroker<E> {
    #handlers = new Map<symbol, (ev: E) => void>();

    listen(handler: (ev: E) => void): CancelListening {
        const id = Symbol();
        this.#handlers.set(id, handler);
        return () => {
            this.#handlers.delete(id);
        };
    }

    hasListeners(): boolean {
        return this.#handlers.size > 0;
    }

    emit(event: E): void {
        for (const handler of this.#handlers.values()) {
            handler(event);
        }
    }
}

export class EventBroker<E> {
    #broker = new CoreBroker<E>();
    #onListenerAdded = new CoreBroker<void>();

    listen(handler: (ev: E) => void): CancelListening {
        this.#onListenerAdded.emit();
        return this.#broker.listen(handler);
    }

    hasListeners(): boolean {
        return this.#broker.hasListeners();
    }

    emit(event: E): void {
        this.#broker.emit(event);
    }

    listenOnListenerAdded(handler: () => void): CancelListening {
        return this.#onListenerAdded.listen(handler);
    }
}

export class SingleListenerEventBroker<E> {
    #handler?: { fn: (ev: E) => void; key: symbol };

    listen(handler: (ev: E) => void): CancelListening {
        if (this.#handler !== undefined) {
            throw new Error("handler is already set");
        }

        const key = Symbol();
        this.#handler = { fn: handler, key };

        return () => {
            if (this.#handler?.key === key) {
                this.#handler = undefined;
            }
        };
    }

    emit(event: E): void {
        this.#handler?.fn(event);
    }
}
