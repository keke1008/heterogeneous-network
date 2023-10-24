import { SerialAddress } from "../address";
import { InitializeResult, SerialReader, SerialWriter, createSerialFrameInterface } from "./common";

class Reader implements SerialReader {
    static async *gen(reader: ReadableStreamDefaultReader<Uint8Array>) {
        while (true) {
            const { value, done } = await reader.read();
            if (done) {
                throw new Error("Stream ended");
            }

            for (const byte of value) {
                yield byte;
            }
        }
    }

    #inner: AsyncGenerator<number, void, void>;

    constructor(reader: ReadableStreamDefaultReader<Uint8Array>) {
        this.#inner = Reader.gen(reader);
    }

    async read(): Promise<number> {
        const { value, done } = await this.#inner.next();
        if (done) {
            throw new Error("Stream ended");
        }
        return value;
    }
}

class Writer implements SerialWriter {
    #writer: WritableStreamDefaultWriter<Uint8Array>;

    constructor(writer: WritableStreamDefaultWriter<Uint8Array>) {
        this.#writer = writer;
    }

    async write(bytes: number | Uint8Array): Promise<void> {
        if (typeof bytes === "number") {
            await this.#writer.write(new Uint8Array([bytes]));
        } else {
            await this.#writer.write(bytes);
        }
    }
}

export const initialize = async (self_address: SerialAddress): Promise<InitializeResult> => {
    const port = await navigator.serial.requestPort().catch((e) => e as DOMException);
    if (port instanceof DOMException) {
        const status = port.name === "SecurityError" ? "permission denied" : "user not selected port";
        return { result: "failure", status };
    }

    if (port.readable === null || port.writable === null) {
        return { result: "failure", status: "already open" };
    }

    const success = await port
        .open({ baudRate: 9600 })
        .then(() => true)
        .catch(() => false);
    if (!success) {
        return { result: "failure", status: "failed to open port" };
    }

    const reader = port.readable.getReader();
    const writer = port.writable.getWriter();
    const finalize = async () => {
        reader.releaseLock();
        writer.releaseLock();
        await port.close();
    };

    return {
        result: "success",
        channel: createSerialFrameInterface(new Reader(reader), new Writer(writer), self_address, finalize),
    };
};
