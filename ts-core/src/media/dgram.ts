import * as dgram from "dgram";
import { Address, BufferReader, BufferWriter, Frame, FrameHandler, UdpAddress, protocolToNumber } from "@core/net";

export class UdpHandler implements FrameHandler {
    #selfAddress: UdpAddress;
    #socket: dgram.Socket;
    #onReceive: undefined | ((frame: Frame) => void) = undefined;
    #onClose: undefined | (() => void) = undefined;

    constructor(address: UdpAddress) {
        this.#selfAddress = address;

        this.#socket = dgram.createSocket("udp4");
        this.#socket.on("message", (data, rinfo) => {
            const sender = new Address(UdpAddress.fromHumanReadableString(rinfo.address, rinfo.port));
            const reader = new BufferReader(data);
            this.#onReceive?.({ protocol: protocolToNumber(reader.readByte()), sender, reader: reader });
        });
        this.#socket.on("listening", () => {
            const address = this.#socket.address();
            console.log(`UDP listening on ${address.address}:${address.port}`);
        });
        this.#socket.on("close", () => this.#onClose?.());
        this.#socket.on("error", (err) => console.log(err));
        this.#socket.bind(this.#selfAddress.#port);
    }

    address(): Address {
        return new Address(this.#selfAddress);
    }

    send(frame: Frame): void {
        if (!(frame.sender.address instanceof UdpAddress)) {
            throw new Error(`Expected UdpAddress, got ${frame.sender}`);
        }

        const writer = new BufferWriter(new Uint8Array(frame.reader.remainingLength() + 1));
        writer.writeByte(frame.protocol);
        writer.writeBytes(frame.reader.readRemaining());

        this.#socket.send(
            writer.unwrapBuffer(),
            frame.sender.address.#port,
            frame.sender.address.humanReadableAddress(),
        );
    }

    onReceive(callback: (frame: Frame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive callback is already set");
        }
        this.#onReceive = callback;
    }

    onClose(callback: () => void): void {
        if (this.#onClose !== undefined) {
            throw new Error("onClose callback is already set");
        }
        this.#onClose = callback;
    }

    close(): void {
        this.#socket.close();
    }
}
