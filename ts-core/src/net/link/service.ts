import { Address, AddressType } from "./address";
import { BufferReader } from "../buffer";
import { Frame, Protocol } from "./frame";

export interface FrameHandler {
    address(): Address;
    send(frame: Frame): void;
    onReceive(callback: (frame: Frame) => void): void;
    onClose(callback: () => void): void;
}

class FrameBroker {
    #handlers: Map<AddressType, FrameHandler> = new Map();
    #onReceive: Map<Protocol, (frame: Frame) => void> = new Map();

    send(frame: Frame): void {
        const handler = this.#handlers.get(frame.sender.type());
        handler?.send(frame);
    }

    addHandler(type: AddressType, handler: FrameHandler): void {
        if (this.#handlers.has(type)) {
            throw new Error(`Handler already set: ${type}`);
        }
        this.#handlers.set(type, handler);
        handler.onReceive((frame) => this.#onReceive.get(frame.protocol)?.(frame));
        handler.onClose(() => this.#handlers.delete(type));
    }

    subscribe(protocol: Protocol, callback: (frame: Frame) => void): void {
        if (this.#onReceive.has(protocol)) {
            throw new Error(`Protocol already subscribed: ${protocol}`);
        }
        this.#onReceive.set(protocol, callback);
    }

    unsubscribe(protocol: Protocol): void {
        this.#onReceive.delete(protocol);
    }
}

export class LinkSocket {
    #protocol: Protocol;
    #broker: FrameBroker;
    #onReceive: ((frame: Frame) => void) | undefined = undefined;

    constructor(protocol: Protocol, broker: FrameBroker) {
        this.#protocol = protocol;
        this.#broker = broker;
        broker.subscribe(protocol, (frame) => this.#onReceive?.(frame));
    }

    send(sender: Address, reader: BufferReader): void {
        const frame = { protocol: this.#protocol, sender, reader };
        this.#broker.send(frame);
    }

    onReceive(callback: (frame: Frame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive already set");
        }
        this.#onReceive = callback;
    }
}

export class LinkService {
    #broker: FrameBroker = new FrameBroker();

    addHandler(type: AddressType, handler: FrameHandler): void {
        this.#broker.addHandler(type, handler);
    }

    open(protocol: Protocol): LinkSocket {
        return new LinkSocket(protocol, this.#broker);
    }
}
