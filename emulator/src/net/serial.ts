import { SerialPort } from "serialport";
import {
    Address,
    BufferReader,
    Frame,
    FrameHandler,
    LinkSendError,
    LinkSendErrorType,
    Protocol,
    SerialAddress,
} from "@core/net";
import { SerialFrameDeserializer, SerialFrameSerializer } from "@core/media/serial";
import { Err, Ok, Result } from "oxide.ts/core";

const BAUD_RATE = 9600;

export type SerialPortPath = string;

// https://serialport.io/docs/api-bindings-cpp#list
interface PortInfo {
    path: SerialPortPath;
    manufacturer: string | undefined;
    serialNumber: string | undefined;
    pnpId: string | undefined;
    locationId: string | undefined;
    productId: string | undefined;
    vendorId: string | undefined;
}

class PortInteractor {
    #port: SerialPort;
    #remoteAddress: SerialAddress;
    #serializer: SerialFrameSerializer = new SerialFrameSerializer();
    #deserializer: SerialFrameDeserializer = new SerialFrameDeserializer();
    #onReceive: (frame: Frame) => void;

    constructor(args: { port: SerialPort; remoteAddress: SerialAddress; onReceive: (frame: Frame) => void }) {
        this.#port = args.port;
        this.#remoteAddress = args.remoteAddress;
        this.#onReceive = args.onReceive;
        this.#port.on("data", (data) => this.#deserializer.dispatchReceivedBytes(data));

        this.#deserializer.onReceive((frame) => {
            console.info("serial received", frame, frame.reader.initialized().readRemaining());
            this.#onReceive?.({
                protocol: frame.protocol,
                remote: new Address(frame.sender),
                reader: frame.reader,
            });
        });
    }

    remoteAddress(): SerialAddress {
        return this.#remoteAddress;
    }

    send(args: { protocol: Protocol; localAddress: SerialAddress; reader: BufferReader }): void {
        console.info("serial sent", args, args.reader.initialized().readRemaining());
        const data = this.#serializer.serialize({
            protocol: args.protocol,
            sender: args.localAddress,
            receiver: this.#remoteAddress,
            reader: args.reader,
        });
        this.#port.write(data);
    }

    close(): void {
        this.#port.close();
    }
}

export class SerialPortStorage {
    #connectedPorts: Map<SerialPortPath, PortInteractor> = new Map();
    #onReceive?: (frame: Frame) => void;

    onReceive(callback: (frame: Frame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive callback already set");
        }
        this.#onReceive = callback;
    }

    async getUnconnectedPorts(): Promise<SerialPortPath[]> {
        const ports: PortInfo[] = await SerialPort.list();
        return ports.filter((port) => !this.#connectedPorts.has(port.path)).map((port) => port.path);
    }

    async getConnectedPorts(): Promise<SerialPortPath[]> {
        return Array.from(this.#connectedPorts.keys());
    }

    async connect(portPath: SerialPortPath, remoteAddress: SerialAddress): Promise<Result<void, Error>> {
        if (this.#connectedPorts.has(portPath)) {
            return Ok(undefined);
        }

        const port = await new Promise<SerialPort>((resove, reject) => {
            const port = new SerialPort({ path: portPath, baudRate: BAUD_RATE }, (error) => reject(error));
            port.on("close", () => this.#connectedPorts.delete(portPath));
            port.on("open", () => resove(port));
        });

        const portInteractor = new PortInteractor({
            port,
            remoteAddress,
            onReceive: (frame) => this.#onReceive?.(frame),
        });
        this.#connectedPorts.set(portPath, portInteractor);

        return Ok(undefined);
    }

    disconnect(portPath: SerialPortPath): void {
        this.#connectedPorts.get(portPath)?.close();
    }

    send(args: {
        protocol: Protocol;
        localAddress: SerialAddress;
        remoteAddress: SerialAddress;
        reader: BufferReader;
    }): void {
        for (const port of this.#connectedPorts.values()) {
            if (port.remoteAddress().equals(args.remoteAddress)) {
                port.send({ protocol: args.protocol, localAddress: args.localAddress, reader: args.reader });
            }
        }
    }

    close(): void {
        for (const port of this.#connectedPorts.values()) {
            port.close();
        }
    }
}

export class SerialHandler implements FrameHandler {
    localAddress: SerialAddress;
    #ports: SerialPortStorage = new SerialPortStorage();
    #onClose?: () => void;

    constructor(localAddress: SerialAddress) {
        this.localAddress = localAddress;
    }

    setLocalAddress(address: SerialAddress): void {
        if (this.localAddress !== undefined) {
            throw new Error("Self address already set");
        }
        this.localAddress = address;
    }

    async getUnconnectedPorts(): Promise<SerialPortPath[]> {
        return this.#ports.getUnconnectedPorts();
    }

    async getConnectedPorts(): Promise<SerialPortPath[]> {
        return this.#ports.getConnectedPorts();
    }

    async connect(portPath: SerialPortPath, remoteAddress: SerialAddress): Promise<Result<void, Error>> {
        return this.#ports.connect(portPath, remoteAddress);
    }

    disconnect(portPath: SerialPortPath): void {
        this.#ports.disconnect(portPath);
    }

    address(): Address {
        return new Address(this.localAddress);
    }

    send(frame: Frame): Result<void, LinkSendError> {
        if (this.localAddress === undefined) {
            return Err({ type: LinkSendErrorType.LocalAddressNotSet });
        }

        const destination = frame.remote.address;
        if (!(destination instanceof SerialAddress)) {
            return Err({ type: LinkSendErrorType.UnsupportedAddressType, addressType: destination.type });
        }

        this.#ports.send({
            protocol: frame.protocol,
            localAddress: this.localAddress,
            remoteAddress: destination,
            reader: frame.reader,
        });

        return Ok(undefined);
    }

    onReceive(callback: (frame: Frame) => void): void {
        this.#ports.onReceive(callback);
    }

    onClose(callback: () => void): void {
        if (this.#onClose !== undefined) {
            throw new Error("onClose callback already set");
        }
        this.#onClose = callback;
    }

    close(): void {
        this.#ports.close();
        this.#onClose?.();
    }
}
