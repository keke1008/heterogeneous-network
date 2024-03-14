import { Address, Frame, FrameHandler, LinkSendError, Protocol, ReceivedFrame, SerialAddress } from "@core/net";
import { Err, Ok, Result } from "oxide.ts";
import { SerialFrameSerializer, SerialFrameDeserializer } from "@media/serial-core";
import { Sender } from "@core/channel";
import { Handle, spawn } from "@core/async";

const BAUD_RATE = 19200;

export class PortAlreadyOpenError extends Error {
    constructor() {
        super("Port already open");
    }
}

class SequentialWriter {
    #handle: Handle<void>;
    #sender = new Sender<Uint8Array>();

    constructor(writer: WritableStreamDefaultWriter<Uint8Array>) {
        const receiver = this.#sender.receiver();
        this.#handle = spawn(async (signal) => {
            signal.addEventListener("abort", () => receiver.close());
            for await (const data of receiver) {
                await writer.write(data);
            }
        });
    }

    write(data: Uint8Array): void {
        this.#sender.send(data);
    }

    async close(): Promise<void> {
        this.#sender.close();
        this.#handle.cancel();
        await this.#handle;
    }
}

export class Port {
    #localAddress: SerialAddress;
    #port: SerialPort;
    #writer: SequentialWriter;
    #serializer: SerialFrameSerializer = new SerialFrameSerializer();
    #deserializer: SerialFrameDeserializer = new SerialFrameDeserializer();

    private constructor(localAddress: SerialAddress, port: SerialPort) {
        this.#localAddress = localAddress;
        this.#port = port;
        this.#writer = new SequentialWriter(port.writable.getWriter());

        (async () => {
            const reader: ReadableStreamDefaultReader<Uint8Array> = port.readable.getReader();
            let data = await reader.read();
            while (!data.done) {
                this.#deserializer.dispatchReceivedBytes(data.value);
                data = await reader.read();
            }
            reader.releaseLock();
        })();
    }

    static async open(localAddress: SerialAddress): Promise<Result<Port, Error | PortAlreadyOpenError>> {
        const resultPort = await Result.safe(navigator.serial.requestPort());
        if (resultPort.isErr()) {
            return resultPort;
        }

        const port = resultPort.unwrap();
        if (port.readable?.locked || port.writable?.locked) {
            return Err(new PortAlreadyOpenError());
        }

        await port.open({ baudRate: BAUD_RATE });
        return Ok(new Port(localAddress, port));
    }

    send(protocol: Protocol, remote: SerialAddress, payload: Uint8Array): void {
        this.#writer.write(
            this.#serializer.serialize({
                protocol: protocol,
                sender: this.#localAddress,
                receiver: remote,
                payload,
            }),
        );
    }

    onReceive(callback: (frame: Frame) => void): void {
        this.#deserializer.onReceive((frame) => {
            callback({
                protocol: frame.protocol,
                payload: frame.payload,
                remote: new Address(frame.sender),
            });
        });
    }

    onCLose(callback: () => void): void {
        this.#port.addEventListener("close", callback);
    }

    close(): void {
        this.#port.close();
    }
}

export class SerialHandler implements FrameHandler {
    #localAddress: SerialAddress;
    #ports: Map<symbol, Port> = new Map();

    #onReceive?: (frame: ReceivedFrame) => void;
    #onClose?: () => void;

    constructor(localAddress: SerialAddress) {
        this.#localAddress = localAddress;
    }

    address(): Address | undefined {
        return new Address(this.#localAddress);
    }

    send(frame: Frame): Result<void, LinkSendError> {
        if (!(frame.remote.address instanceof SerialAddress)) {
            return Err({ type: "unsupported address type", addressType: frame.remote.type() });
        }

        for (const port of this.#ports.values()) {
            port.send(frame.protocol, frame.remote.address, frame.payload);
        }
        return Ok(undefined);
    }

    onReceive(callback: (frame: ReceivedFrame) => void): void {
        if (this.#onReceive) {
            throw new Error("onReceive callback already set");
        }
        this.#onReceive = callback;
    }

    onClose(callback: () => void): void {
        if (this.#onClose) {
            throw new Error("onClose callback already set");
        }
        this.#onClose = callback;
    }

    async addPort(): Promise<Result<void, Error | PortAlreadyOpenError>> {
        const resultPort = await Port.open(this.#localAddress);
        if (resultPort.isErr()) {
            return resultPort;
        }

        const key = Symbol();
        const portObj = resultPort.unwrap();
        this.#ports.set(key, portObj);

        const abortController = new AbortController();
        portObj.onReceive((frame) => this.#onReceive?.({ ...frame, mediaPortAbortSignal: abortController.signal }));
        portObj.onCLose(() => {
            this.#ports.delete(key);
            abortController.abort();
        });

        return Ok(undefined);
    }
}
