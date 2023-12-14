import { Address, AddressType, LoopbackAddress } from "./address";
import { BufferReader } from "../buffer";
import { Frame, Protocol } from "./frame";
import { Err, Ok, Result } from "oxide.ts";
import { SingleListenerEventBroker } from "@core/event";

export const LinkSendErrorType = {
    UnsupportedAddressType: "unsupported address type",
    FrameHandlerNotRegistered: "frame handler not registered",
    Other: "other",
} as const;
export type LinkSendErrorType = (typeof LinkSendErrorType)[keyof typeof LinkSendErrorType];
export type LinkSendError =
    | { type: typeof LinkSendErrorType.FrameHandlerNotRegistered; addressType: AddressType }
    | { type: typeof LinkSendErrorType.UnsupportedAddressType; addressType: AddressType }
    | { type: typeof LinkSendErrorType.Other; error: unknown };

export const LinkBroadcastErrorType = {
    UnsupportedAddressType: "unsupported address type",
    Other: "other",
} as const;
export type LinkBroadcastErrorType = (typeof LinkBroadcastErrorType)[keyof typeof LinkBroadcastErrorType];
export type LinkBroadcastError =
    | { type: typeof LinkBroadcastErrorType.UnsupportedAddressType; addressType: AddressType }
    | { type: typeof LinkBroadcastErrorType.Other; error: unknown };

export interface FrameHandler {
    address(): Address | undefined;
    send(frame: Frame): Result<void, LinkSendError>;
    sendBroadcast?(protocol: Protocol, payload: BufferReader): Result<void, LinkBroadcastError>;
    onReceive(callback: (frame: Frame) => void): void;
    onClose(callback: () => void): void;
}

class FrameBroker {
    #handlers: Map<AddressType, FrameHandler> = new Map();
    #onReceive: Map<Protocol, (frame: Frame) => void> = new Map();

    send(frame: Frame): Result<void, LinkSendError> {
        const handler = this.#handlers.get(frame.remote.type());
        if (handler === undefined) {
            return Err({
                type: LinkSendErrorType.FrameHandlerNotRegistered,
                addressType: frame.remote.type(),
            });
        }
        return handler.send(frame);
    }

    sendBroadcast(type: AddressType, protocol: Protocol, reader: BufferReader): Result<void, LinkBroadcastError> {
        const handler = this.#handlers.get(type);
        if (handler?.sendBroadcast === undefined) {
            return Err({
                type: LinkSendErrorType.UnsupportedAddressType,
                addressType: type,
            });
        }
        return handler.sendBroadcast(protocol, reader);
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

    supportedAddressTypes(): AddressType[] {
        return Array.from(this.#handlers.keys());
    }

    getAddresses(): Address[] {
        return Array.from(this.#handlers.values())
            .map((handler) => handler.address())
            .filter((address) => address !== undefined) as Address[];
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

    send(remote: Address, reader: BufferReader): Result<void, LinkSendError> {
        const frame = { protocol: this.#protocol, remote, reader };
        return this.#broker.send(frame);
    }

    sendBroadcast(type: AddressType, reader: BufferReader): Result<void, LinkBroadcastError> {
        return this.#broker.sendBroadcast(type, this.#protocol, reader);
    }

    onReceive(callback: (frame: Frame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive already set");
        }
        this.#onReceive = callback;
    }

    supportedAddressTypes(): AddressType[] {
        return this.#broker.supportedAddressTypes();
    }
}

class LoopbackHandler implements FrameHandler {
    #onReceive = new SingleListenerEventBroker<Frame>();

    address(): Address | undefined {
        return new Address(new LoopbackAddress());
    }

    send(frame: Frame): Result<void, LinkSendError> {
        this.#onReceive.emit(frame);
        return Ok(undefined);
    }

    onReceive(callback: (frame: Frame) => void): void {
        this.#onReceive.listen(callback);
    }

    onClose(): void {}
}

export class LinkService {
    #broker: FrameBroker = new FrameBroker();
    #onAddressAdded = new SingleListenerEventBroker<Address>();

    constructor() {
        this.#broker.addHandler(AddressType.Loopback, new LoopbackHandler());
    }

    addHandler(type: AddressType, handler: FrameHandler): void {
        this.#broker.addHandler(type, handler);

        const address = handler.address();
        if (address !== undefined) {
            this.#onAddressAdded.emit(address);
        }
    }

    open(protocol: Protocol): LinkSocket {
        return new LinkSocket(protocol, this.#broker);
    }

    onAddressAdded(callback: (address: Address) => void): void {
        this.#onAddressAdded.listen(callback);
    }

    getLocalAddresses(): Address[] {
        return this.#broker.getAddresses();
    }
}
