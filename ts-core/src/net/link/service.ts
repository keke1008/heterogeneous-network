import { Address, AddressType, LoopbackAddress } from "./address";
import { FRAME_MTU, Frame, Protocol, ReceivedFrame } from "./frame";
import { Err, Ok, Result } from "oxide.ts";
import { SingleListenerEventBroker } from "@core/event";
import { Sender } from "@core/channel";
import { Handle, sleep, spawn } from "@core/async";
import { SEND_INTERVAL } from "./constants";
import { Instant } from "@core/time";

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
    sendBroadcast?(protocol: Protocol, payload: Uint8Array): Result<void, LinkBroadcastError>;
    onReceive(callback: (frame: ReceivedFrame) => void): void;
    onClose(callback: () => void): void;
}

class FrameBroker {
    #handlers: Map<AddressType, FrameHandler> = new Map();
    #sender: Sender<[Frame, (result: Result<void, LinkSendError>) => void]>;
    #senderHandle: Handle<void>;
    #onReceive: Map<Protocol, (frame: ReceivedFrame) => void> = new Map();

    constructor() {
        this.#sender = new Sender();
        this.#senderHandle = spawn(async () => {
            let sentInstant: Instant | undefined = undefined;
            for await (const [frame, result] of this.#sender.receiver()) {
                if (sentInstant !== undefined) {
                    const delay = sentInstant.elapsed();
                    if (delay.lessThan(SEND_INTERVAL)) {
                        await sleep(SEND_INTERVAL.subtract(delay));
                    }
                }

                const handler = this.#handlers.get(frame.remote.type());
                if (handler === undefined) {
                    result(
                        Err({
                            type: LinkSendErrorType.FrameHandlerNotRegistered,
                            addressType: frame.remote.type(),
                        }),
                    );
                    continue;
                }

                result(handler.send(frame));
                sentInstant = Instant.now();
            }
        });
    }

    async send(frame: Frame): Promise<Result<void, LinkSendError>> {
        if (frame.payload.length > FRAME_MTU) {
            throw new Error(`Payload too large: ${frame.payload.length}`);
        }

        return new Promise((resolve) => this.#sender.send([frame, resolve]));
    }

    sendBroadcast(type: AddressType, protocol: Protocol, payload: Uint8Array): Result<void, LinkBroadcastError> {
        if (payload.length > FRAME_MTU) {
            throw new Error(`Payload too large: ${payload.length}`);
        }

        const handler = this.#handlers.get(type);
        if (handler?.sendBroadcast === undefined) {
            return Err({
                type: LinkSendErrorType.UnsupportedAddressType,
                addressType: type,
            });
        }
        return handler.sendBroadcast(protocol, payload);
    }

    addHandler(type: AddressType, handler: FrameHandler): void {
        if (this.#handlers.has(type)) {
            throw new Error(`Handler already set: ${type}`);
        }
        this.#handlers.set(type, handler);
        handler.onReceive((frame) => this.#onReceive.get(frame.protocol)?.(frame));
        handler.onClose(() => this.#handlers.delete(type));
    }

    subscribe(protocol: Protocol, callback: (frame: ReceivedFrame) => void): void {
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

    async close(): Promise<void> {
        this.#sender.close();
        await this.#senderHandle;
    }
}

export class LinkSocket {
    #protocol: Protocol;
    #broker: FrameBroker;
    #onReceive: ((frame: ReceivedFrame) => void) | undefined = undefined;

    constructor(protocol: Protocol, broker: FrameBroker) {
        this.#protocol = protocol;
        this.#broker = broker;
        broker.subscribe(protocol, (frame) => this.#onReceive?.(frame));
    }

    maxPayloadLength(): number {
        return FRAME_MTU;
    }

    send(remote: Address, payload: Uint8Array): Promise<Result<void, LinkSendError>> {
        if (payload.length > FRAME_MTU) {
            throw new Error(`Payload too large: ${payload.length}`);
        }

        const frame = { protocol: this.#protocol, remote, payload };
        return this.#broker.send(frame);
    }

    sendBroadcast(type: AddressType, payload: Uint8Array): Result<void, LinkBroadcastError> {
        return this.#broker.sendBroadcast(type, this.#protocol, payload);
    }

    onReceive(callback: (frame: ReceivedFrame) => void): void {
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
    #onReceive = new SingleListenerEventBroker<ReceivedFrame>();
    readonly #abortController = new AbortController();

    address(): Address | undefined {
        return new Address(new LoopbackAddress());
    }

    send(frame: Frame): Result<void, LinkSendError> {
        this.#onReceive.emit({ ...frame, mediaPortAbortSignal: this.#abortController.signal });
        return Ok(undefined);
    }

    onReceive(callback: (frame: ReceivedFrame) => void): void {
        this.#onReceive.listen(callback);
    }

    onClose(): void {
        this.#abortController.abort();
    }
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
