import { BufferReader } from "../buffer";
import { Address, SerialAddress } from "../address";
import { Frame, FrameInterface, Protocol, numberToProtocol, protocolToNumber } from "../frame";
import { Sender } from "src/lib/channel";

const PREAMBLE = 0b10101010;
const PREAMBLE_LENGTH = 8;

interface SerialFrame {
    protocol: Protocol;
    peer: SerialAddress;
    dest: SerialAddress;
    reader: BufferReader;
}

export interface SerialReader {
    read(): Promise<number>;
}

export interface SerialWriter {
    write(byte: number | Uint8Array): Promise<void>;
}

const receivePreamble = async (reader: SerialReader) => {
    let count = 0;
    while (count < PREAMBLE_LENGTH) {
        const byte = await reader.read();
        count = byte === PREAMBLE ? count + 1 : 0;
    }
};

const receiveBody = async (reader: SerialReader, length: number): Promise<BufferReader> => {
    const buffer = new Uint8Array(length);
    for (let i = 0; i < length; i++) {
        buffer[i] = await reader.read();
    }
    return new BufferReader(buffer);
};

const receiveSerialFrame = async (reader: SerialReader): Promise<SerialFrame> => {
    await receivePreamble(reader);
    return {
        protocol: numberToProtocol(await reader.read()),
        peer: new SerialAddress(await reader.read()),
        dest: new SerialAddress(await reader.read()),
        reader: await receiveBody(reader, await reader.read()),
    };
};

const sendSerialFrame = async (writer: SerialWriter, frame: SerialFrame) => {
    for (let i = 0; i < PREAMBLE_LENGTH; i++) {
        await writer.write(PREAMBLE);
    }

    await writer.write(protocolToNumber(frame.protocol));
    await writer.write(frame.peer.address());
    await writer.write(frame.dest.address());
    await writer.write(frame.reader.readableLength());
    await writer.write(frame.reader.readRemaining());
};

export const createSerialFrameInterface = (
    reader: SerialReader,
    writer: SerialWriter,
    selfAddress: SerialAddress,
    finalize: () => void,
): FrameInterface => {
    const sendTx = new Sender<Frame>();
    sendTx.receiver().forEach((frame) => {
        if (!(frame.peer instanceof SerialAddress)) {
            throw new Error(`Cannot send frame to non-serial address: ${frame.peer}`);
        }
        sendSerialFrame(writer, {
            protocol: frame.protocol,
            peer: selfAddress,
            dest: new SerialAddress(frame.peer.address()),
            reader: frame.reader,
        });
    });

    const receiveTx = new Sender<Frame>();
    (async () => {
        while (!receiveTx.isClosed()) {
            const { protocol, peer, dest, reader: frame_reader } = await receiveSerialFrame(reader);
            if (!dest.equals(selfAddress)) {
                continue;
            }

            const frame: Frame = { protocol, peer: new Address(peer), reader: frame_reader };
            receiveTx.send(frame);
        }
    })();

    return new FrameInterface(sendTx, receiveTx.receiver(), finalize);
};

export type InitializeResult =
    | { result: "success"; channel: FrameInterface }
    | { result: "failure"; status: "permission denied" }
    | { result: "failure"; status: "user not selected port" }
    | { result: "failure"; status: "already open" }
    | { result: "failure"; status: "failed to open port" };
