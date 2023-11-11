import { Address, Frame, FrameHandler, SerialAddress } from "@core/net";
import { SerialFrameDeserializer, serializeFrame } from "./frame";

export interface Port {
    send: (data: Uint8Array) => void;
    onReceive: (callback: (data: Uint8Array) => void) => void;
    onClose: (callback: () => void) => void;
    close: () => void;
}

export class SerialHandler implements FrameHandler {
    #selfAddress: SerialAddress;
    #port: Port;
    #deserializer: SerialFrameDeserializer = new SerialFrameDeserializer();

    constructor(selfAddress: SerialAddress, port: Port) {
        this.#selfAddress = selfAddress;
        this.#port = port;
    }

    address(): Address {
        return new Address(this.#selfAddress);
    }

    send(frame: Frame): void {
        const data = serializeFrame({
            protocol: frame.protocol,
            sender: this.#selfAddress,
            receiver: this.#selfAddress,
            reader: frame.reader,
        });
        this.#port.send(data);
    }

    onReceive(callback: (frame: Frame) => void): void {
        this.#deserializer.onReceive((frame) => {
            callback({
                protocol: frame.protocol,
                sender: new Address(frame.sender),
                reader: frame.reader.initialized(),
            });
        });
    }

    onClose(callback: () => void): void {
        this.#port.onClose(callback);
    }

    close(): void {
        this.#port.close();
    }
}
