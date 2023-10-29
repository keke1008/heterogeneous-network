type Queue<T> = T[];
type Handler<T> = (event: T) => void;

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
