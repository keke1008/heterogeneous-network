import * as dgram from "dgram";
import { Address, BufferReader, Frame, FrameHandler, SinetAddress, protocolToNumber } from "@core/net";

export class UdpHandler implements FrameHandler {
    #selfAddress: SinetAddress;
    #socket: dgram.Socket;
    #onReceive: undefined | ((frame: Frame) => void) = undefined;
    #onClose: undefined | (() => void) = undefined;

    constructor(address: SinetAddress) {
        this.#selfAddress = address;

        this.#socket = dgram.createSocket("udp4");
        this.#socket.on("message", (data, rinfo) => {
            const sender = new Address(SinetAddress.fromHumanReadableString(rinfo.address, rinfo.port));
            const reader = new BufferReader(data);
            this.#onReceive?.({ protocol: protocolToNumber(reader.readByte()), sender, reader: reader });
        });
        this.#socket.on("listening", () => console.log(`UDP listening on port ${address.port}`));
        this.#socket.on("close", () => this.#onClose?.());
        this.#socket.on("error", (err) => console.log(err));
    }

    bind(): void {
        this.#socket.bind(this.#selfAddress.port);
    }

    address(): Address {
        return new Address(this.#selfAddress);
    }

    send(frame: Frame): void {
        if (!(frame.sender.address instanceof SinetAddress)) {
            throw new Error(`Expected SinetAddress, got ${frame.sender}`);
        }

        this.#socket.send(
            frame.reader.readRemaining(),
            frame.sender.address.port,
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
