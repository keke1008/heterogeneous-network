import * as dgram from "node:dgram";
import * as os from "node:os";
import * as z from "zod";
import {
    Address,
    BufferReader,
    BufferWriter,
    Frame,
    FrameHandler,
    LinkSendError,
    PROTOCOL_LENGTH,
    Protocol,
    UdpAddress,
    protocolToNumber,
} from "@core/net";
import { Err, Ok, Result } from "oxide.ts";
import { DeserializeResult, InvalidValueError } from "@core/serde";
import { P, match } from "ts-pattern";
import { ObjectSet } from "@core/object";
import { EventBroker, SingleListenerEventBroker } from "@core/event";

export const getLocalIpV4Addresses = (): string[] => {
    const schema = z.string().ip({ version: "v4" });
    const interfaces = os.networkInterfaces();
    return [...Object.values(interfaces)]
        .flat()
        .filter((i): i is Exclude<typeof i, undefined> => i !== undefined && i.family == "IPv4" && i.internal == false)
        .map((i) => schema.parse(i.address));
};

class GlobalAddressStore {
    #reqeusted = new ObjectSet<UdpAddress, string>((address) => address.humanReadableAddress());
    #globalAddress?: UdpAddress;

    #sendControlFrame(destination: UdpAddress, frame: ControlFrame, socket: dgram.Socket): void {
        const udpFrame = new UdpFrame(frame);
        const writer = new BufferWriter(new Uint8Array(udpFrame.serializedLength()));
        udpFrame.serialize(writer);
        socket.send(writer.unwrapBuffer(), destination.port(), destination.humanReadableAddress());
    }

    globalAddress(): UdpAddress | undefined {
        return this.#globalAddress;
    }

    onSendDataFrame(address: UdpAddress, socket: dgram.Socket): void {
        if (this.#reqeusted.has(address)) {
            return;
        }

        this.#sendControlFrame(address, new GlobalAddressRequestFrame(), socket);
        this.#reqeusted.add(address);
    }

    onReceiveControlFrame(source: UdpAddress, frame: ControlFrame, socket: dgram.Socket): void {
        match(frame)
            .with(P.instanceOf(GlobalAddressRequestFrame), () => {
                this.#sendControlFrame(source, new GlobalAddressResponseFrame({ address: source }), socket);
            })
            .with(P.instanceOf(GlobalAddressResponseFrame), (frame) => {
                const address = frame.address.humanReadableAddress();
                if (this.#globalAddress === undefined && !getLocalIpV4Addresses().includes(address)) {
                    this.#globalAddress = frame.address;
                }
            })
            .exhaustive();
    }
}

enum FrameType {
    Data = 1,
    GlobalAddressRequest = 2,
    GlobalAddressResponse = 3,
}

const FRAME_TYPE_LENGTH = 1;

const serializeFrameType = (reader: BufferReader): DeserializeResult<FrameType> => {
    const type = reader.readByte();
    return type in FrameType ? Ok(type as FrameType) : Err(new InvalidValueError());
};

const deserializeFrameType = (writer: BufferWriter, type: FrameType): void => {
    writer.writeByte(type);
};

class DataFrame {
    type: FrameType.Data = FrameType.Data as const;
    protocol: Protocol;
    reader: Uint8Array;

    constructor(args: { protocol: Protocol; reader: Uint8Array }) {
        this.protocol = args.protocol;
        this.reader = args.reader;
    }

    static deserialize(reader: BufferReader): DeserializeResult<DataFrame> {
        const protocol = protocolToNumber(reader.readByte());
        return Ok(new DataFrame({ protocol: protocol, reader: reader.readRemaining() }));
    }

    serialize(writer: BufferWriter): void {
        deserializeFrameType(writer, this.type);
        writer.writeByte(this.protocol);
        writer.writeBytes(this.reader);
    }

    serializedLength(): number {
        return FRAME_TYPE_LENGTH + PROTOCOL_LENGTH + this.reader.length;
    }
}

class GlobalAddressRequestFrame {
    type: FrameType.GlobalAddressRequest = FrameType.GlobalAddressRequest as const;

    static deserialize(): DeserializeResult<GlobalAddressRequestFrame> {
        return Ok(new GlobalAddressRequestFrame());
    }

    serialize(writer: BufferWriter): void {
        deserializeFrameType(writer, this.type);
    }

    serializedLength(): number {
        return FRAME_TYPE_LENGTH;
    }
}

class GlobalAddressResponseFrame {
    type: FrameType.GlobalAddressResponse = FrameType.GlobalAddressResponse as const;
    address: UdpAddress;

    constructor(args: { address: UdpAddress }) {
        this.address = args.address;
    }

    static deserialize(reader: BufferReader): DeserializeResult<GlobalAddressResponseFrame> {
        return UdpAddress.deserialize(reader).map((address) => new GlobalAddressResponseFrame({ address }));
    }

    serialize(writer: BufferWriter): void {
        deserializeFrameType(writer, this.type);
        this.address.serialize(writer);
    }

    serializedLength(): number {
        return FRAME_TYPE_LENGTH + this.address.serializedLength();
    }
}

type ControlFrame = GlobalAddressRequestFrame | GlobalAddressResponseFrame;

class UdpFrame {
    frame: DataFrame | ControlFrame;

    constructor(frame: DataFrame | ControlFrame) {
        this.frame = frame;
    }

    static deserialize(reader: BufferReader): DeserializeResult<UdpFrame> {
        const frameType = serializeFrameType(reader).unwrap();
        const frameClasses = {
            [FrameType.Data]: DataFrame,
            [FrameType.GlobalAddressRequest]: GlobalAddressRequestFrame,
            [FrameType.GlobalAddressResponse]: GlobalAddressResponseFrame,
        } as const;
        const result: DeserializeResult<DataFrame | ControlFrame> = frameClasses[frameType].deserialize(reader);
        return result.map((frame) => new UdpFrame(frame));
    }

    serialize(writer: BufferWriter): void {
        this.frame.serialize(writer);
    }

    serializedLength(): number {
        return this.frame.serializedLength();
    }
}

export class UdpHandler implements FrameHandler {
    #selfPort: number;
    #globalAddress = new GlobalAddressStore();
    #socket: dgram.Socket;
    #onReceive = new SingleListenerEventBroker<Frame>();
    #onClose = new EventBroker<void>();

    constructor(port: number) {
        this.#selfPort = port;

        this.#socket = dgram.createSocket("udp4");
        this.#socket.on("message", (data, rinfo) => {
            const source = UdpAddress.fromHumanReadableString(rinfo.address, rinfo.port).expect("Invalid address");
            const frame = UdpFrame.deserialize(new BufferReader(data));
            if (frame.isErr()) {
                console.warn(`Failed to deserialize frame: ${frame.unwrapErr()}`);
                return;
            }

            match(frame.unwrap().frame)
                .with(P.instanceOf(DataFrame), (frame) => {
                    this.#onReceive.emit({
                        protocol: frame.protocol,
                        remote: new Address(source),
                        reader: new BufferReader(frame.reader),
                    });
                })
                .with(
                    P.union(P.instanceOf(GlobalAddressRequestFrame), P.instanceOf(GlobalAddressResponseFrame)),
                    (frame) => this.#globalAddress.onReceiveControlFrame(source, frame, this.#socket),
                )
                .exhaustive();
        });
        this.#socket.on("listening", () => {
            const address = this.#socket.address();
            console.log(`UDP listening on ${address.address}:${address.port}`);
        });
        this.#socket.on("close", () => this.#onClose.emit());
        this.#socket.on("error", (err) => console.log(err));
        this.#socket.bind(this.#selfPort);
    }

    address(): Address | undefined {
        const address = this.#globalAddress.globalAddress();
        return address && new Address(address);
    }

    send(frame: Frame): Result<void, LinkSendError> {
        if (!(frame.remote.address instanceof UdpAddress)) {
            throw new Error(`Expected UdpAddress, got ${frame.remote}`);
        }

        const writer = new BufferWriter(new Uint8Array(frame.reader.remainingLength() + 1));
        writer.writeByte(frame.protocol);
        writer.writeBytes(frame.reader.readRemaining());

        this.#socket.send(
            writer.unwrapBuffer(),
            frame.remote.address.port(),
            frame.remote.address.humanReadableAddress(),
        );
        this.#globalAddress.onSendDataFrame(frame.remote.address, this.#socket);
        return Ok(undefined);
    }

    onReceive(callback: (frame: Frame) => void): void {
        this.#onReceive.listen(callback);
    }

    onClose(callback: () => void): void {
        this.#onClose.listen(callback);
    }

    close(): void {
        this.#socket.close();
    }
}
