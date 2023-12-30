import * as dgram from "node:dgram";
import * as os from "node:os";
import * as z from "zod";
import { Ok, Result } from "oxide.ts";
import {
    Address,
    BufferReader,
    BufferWriter,
    Frame,
    FrameHandler,
    LinkSendError,
    Protocol,
    ProtocolSerdeable,
    UdpAddress,
} from "@core/net";
import {
    ConstantSerdeable,
    ObjectSerdeable,
    RemainingBytesSerdeable,
    TransformSerdeable,
    VariantSerdeable,
} from "@core/serde";
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
    #reqeusted = new ObjectSet<UdpAddress>();
    #globalAddress?: UdpAddress;

    #sendControlFrame(destination: UdpAddress, frame: ControlFrame, socket: dgram.Socket): void {
        const buffer = BufferWriter.serialize(UdpFrame.serdeable.serializer(frame)).expect("Failed to serialize frame");
        socket.send(buffer, destination.port(), destination.humanReadableAddress());
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

class DataFrame {
    type: FrameType.Data = FrameType.Data as const;
    protocol: Protocol;
    payload: Uint8Array;

    constructor(args: { protocol: Protocol; payload: Uint8Array }) {
        this.protocol = args.protocol;
        this.payload = args.payload;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ protocol: ProtocolSerdeable, payload: new RemainingBytesSerdeable() }),
        (obj) => new DataFrame(obj),
        (frame) => frame,
    );
}

class GlobalAddressRequestFrame {
    type: FrameType.GlobalAddressRequest = FrameType.GlobalAddressRequest as const;

    static readonly serdeable = new ConstantSerdeable(new GlobalAddressRequestFrame());
}

class GlobalAddressResponseFrame {
    type: FrameType.GlobalAddressResponse = FrameType.GlobalAddressResponse as const;
    address: UdpAddress;

    constructor(args: { address: UdpAddress }) {
        this.address = args.address;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ address: UdpAddress.serdeable }),
        (obj) => new GlobalAddressResponseFrame(obj),
        (frame) => frame,
    );
}

type ControlFrame = GlobalAddressRequestFrame | GlobalAddressResponseFrame;

type UdpFrame = DataFrame | ControlFrame;

const UdpFrame = {
    serdeable: new VariantSerdeable(
        [DataFrame.serdeable, GlobalAddressRequestFrame.serdeable, GlobalAddressResponseFrame.serdeable],
        (frame) => frame.type,
    ),
} as const;

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
            const source = UdpAddress.schema.parse([rinfo.address, rinfo.port]);
            const frame = UdpFrame.serdeable.deserializer().deserialize(new BufferReader(data));
            if (frame.isErr()) {
                console.warn(`Failed to deserialize frame: ${frame.unwrapErr()}`);
                return;
            }

            match(frame.unwrap())
                .with(P.instanceOf(DataFrame), (frame) => {
                    this.#onReceive.emit({
                        protocol: frame.protocol,
                        remote: new Address(source),
                        payload: frame.payload,
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

        const dataFrame = new DataFrame({ protocol: frame.protocol, payload: frame.payload });
        this.#socket.send(
            BufferWriter.serialize(UdpFrame.serdeable.serializer(dataFrame)).expect("Failed to serialize frame"),
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
