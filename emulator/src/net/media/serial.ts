import { Address, BufferReader, Frame, FrameHandler, LinkSendError, Protocol, SerialAddress } from "@core/net";
import { Err, Ok, Result } from "oxide.ts/core";
import { SerialFrameSerializer, SerialFrameDeserializer } from "@core/media/serial";

const BAUD_RATE = 9600;

class PortAlreadyOpenError extends Error {
    constructor() {
        super("Port already open");
    }
}

export class Port {
    #localAddress: SerialAddress;
    #port: SerialPort;
    #writer: WritableStreamDefaultWriter<Uint8Array>;
    #serializer: SerialFrameSerializer = new SerialFrameSerializer();
    #deserializer: SerialFrameDeserializer = new SerialFrameDeserializer();

    private constructor(localAddress: SerialAddress, port: SerialPort) {
        this.#localAddress = localAddress;
        this.#port = port;
        this.#writer = port.writable.getWriter();

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

    static async open(localAddress: SerialAddress): Promise<Result<Port, unknown>> {
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

    send(protocol: Protocol, remote: SerialAddress, reader: BufferReader): void {
        this.#writer.write(
            this.#serializer.serialize({
                protocol: protocol,
                sender: this.#localAddress,
                receiver: remote,
                reader: reader,
            }),
        );
    }

    onReceive(callback: (frame: Frame) => void): void {
        this.#deserializer.onReceive((frame) => {
            callback({
                protocol: frame.protocol,
                reader: frame.reader,
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

    #onReceive?: (frame: Frame) => void;
    #onClose?: () => void;

    constructor(localAddress: SerialAddress) {
        this.#localAddress = localAddress;
    }

    address(): Address | undefined {
        return new Address(this.#localAddress);
    }

    send(frame: Frame): Result<void, LinkSendError> {
        if (!(frame.remote instanceof SerialAddress)) {
            return Err({ type: "unsupported address type", addressType: frame.remote.type() });
        }

        for (const port of this.#ports.values()) {
            port.send(frame.protocol, frame.remote, frame.reader.initialized());
        }
        return Ok(undefined);
    }

    onReceive(callback: (frame: Frame) => void): void {
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

    async addPort(): Promise<Result<void, unknown>> {
        const resultPort = await Port.open(this.#localAddress);
        if (resultPort.isErr()) {
            return resultPort;
        }

        const key = Symbol();
        const portObj = resultPort.unwrap();
        this.#ports.set(key, portObj);
        portObj.onReceive((frame) => this.#onReceive?.(frame));
        portObj.onCLose(() => this.#ports.delete(key));
        return Ok(undefined);
    }
}
