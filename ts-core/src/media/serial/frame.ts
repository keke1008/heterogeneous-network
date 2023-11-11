import { BufferReader, FRAME_MTU, Protocol, SerialAddress, numberToProtocol, protocolToNumber } from "@core/net";

const PREAMBLE = 0b10101010;
const PREAMBLE_LENGTH = 8;

interface SerialFrameHeader {
    protocol: Protocol;
    sender: SerialAddress;
    receiver: SerialAddress;
    length: number;
}

interface SerialFrame {
    protocol: Protocol;
    sender: SerialAddress;
    receiver: SerialAddress;
    reader: BufferReader;
}

function* replaceEmptyReader(reader: BufferReader): Generator<void, BufferReader, BufferReader> {
    while (reader.remainingLength() === 0) {
        reader = yield;
    }
    return reader;
}

function* deserializePreamble(reader: BufferReader): Generator<void, BufferReader, BufferReader> {
    let count = 0;
    while (count < PREAMBLE_LENGTH) {
        reader = yield* replaceEmptyReader(reader);

        const restPreambleLength = PREAMBLE_LENGTH - count;
        const bytes = reader.readBytesMax(restPreambleLength);
        count = bytes.every((byte) => byte === PREAMBLE) ? count + bytes.length : 0;
    }

    return reader;
}

function* deserializeHeader(reader: BufferReader): Generator<void, [BufferReader, SerialFrameHeader], BufferReader> {
    function* readByte(): Generator<void, number, BufferReader> {
        return (yield* replaceEmptyReader(reader)).readByte();
    }

    const header = {
        protocol: numberToProtocol(yield* readByte()),
        sender: new SerialAddress(yield* readByte()),
        receiver: new SerialAddress(yield* readByte()),
        length: yield* readByte(),
    };
    return [reader, header];
}

function* deserializeBody(
    length: number,
    reader: BufferReader,
): Generator<void, [BufferReader, Uint8Array], BufferReader> {
    const body = new Uint8Array(length);
    let offset = 0;

    while (offset < length) {
        reader = yield* replaceEmptyReader(reader);

        const restLength = length - offset;
        const bytes = reader.readBytesMax(restLength);
        body.set(bytes, offset);
        offset += bytes.length;
    }

    return [reader, body];
}

function* deserializeFrame(reader: BufferReader): Generator<void, [BufferReader, SerialFrame], BufferReader> {
    const reader1 = yield* deserializePreamble(reader);
    const [reader2, header] = yield* deserializeHeader(reader1);
    const [reader3, body] = yield* deserializeBody(reader1.remainingLength(), reader2);
    const frame = {
        protocol: header.protocol,
        sender: header.sender,
        receiver: header.receiver,
        reader: new BufferReader(body),
    };
    return [reader3, frame];
}

export class SerialFrameDeserializer {
    #gen?: Generator<void, [BufferReader, SerialFrame], BufferReader>;
    #onReceive?: (frame: SerialFrame) => void;

    onReceive(callback: (frame: SerialFrame) => void): void {
        if (this.#onReceive !== undefined) {
            throw new Error("onReceive callback already set");
        }
        this.#onReceive = callback;
    }

    dispatchReceivedBytes(reader: BufferReader): void {
        let next = this.#gen?.next(reader);
        while (next?.done) {
            const frame = next.value[1];
            this.#onReceive?.(frame);
            this.#gen = deserializeFrame(reader);
        }
    }
}

export const serializeFrame = (frame: SerialFrame): Uint8Array => {
    const preamble = Array(PREAMBLE_LENGTH).fill(PREAMBLE);
    const header = [
        protocolToNumber(frame.protocol),
        frame.sender.address(),
        frame.receiver.address(),
        frame.reader.remainingLength(),
    ];
    const body = frame.reader.readRemaining();

    const data = new Uint8Array([...preamble, ...header, ...body]);
    if (data.length > FRAME_MTU) {
        throw new Error("Frame too large");
    }
    return data;
};
