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
    ReceivedFrame,
    UdpAddress,
} from "@core/net";
import { ObjectSerdeable, RemainingBytesSerdeable, TransformSerdeable } from "@core/serde";
import { EventBroker, SingleListenerEventBroker } from "@core/event";

export const getLocalIpV4Addresses = (): string[] => {
    const schema = z.string().ip({ version: "v4" });
    const interfaces = os.networkInterfaces();
    return [...Object.values(interfaces)]
        .flat()
        .filter((i): i is Exclude<typeof i, undefined> => i !== undefined && i.family == "IPv4" && i.internal == false)
        .map((i) => schema.parse(i.address));
};

class UdpFrame {
    protocol: Protocol;
    payload: Uint8Array;

    constructor(args: { protocol: Protocol; payload: Uint8Array }) {
        this.protocol = args.protocol;
        this.payload = args.payload;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ protocol: ProtocolSerdeable, payload: new RemainingBytesSerdeable() }),
        (obj) => new UdpFrame(obj),
        (frame) => frame,
    );
}

export class UdpHandler implements FrameHandler {
    #selfPort: number;
    #socket: dgram.Socket;
    #onReceive = new SingleListenerEventBroker<ReceivedFrame>();
    #onClose = new EventBroker<void>();
    readonly #abortController = new AbortController();

    constructor(port: number) {
        this.#selfPort = port;

        this.#socket = dgram.createSocket("udp4");
        this.#socket.on("message", (data, rinfo) => {
            const source = UdpAddress.schema.parse([rinfo.address, rinfo.port]);
            const result = UdpFrame.serdeable.deserializer().deserialize(new BufferReader(data));
            if (result.isErr()) {
                console.warn(`Failed to deserialize frame: ${result.unwrapErr()}`);
                return;
            }

            const frame = result.unwrap();
            this.#onReceive.emit({
                protocol: frame.protocol,
                remote: new Address(source),
                payload: frame.payload,
                mediaPortAbortSignal: this.#abortController.signal,
            });
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
        return undefined;
    }

    send(frame: Frame): Result<void, LinkSendError> {
        if (!(frame.remote.address instanceof UdpAddress)) {
            throw new Error(`Expected UdpAddress, got ${frame.remote}`);
        }

        const dataFrame = new UdpFrame({ protocol: frame.protocol, payload: frame.payload });
        this.#socket.send(
            BufferWriter.serialize(UdpFrame.serdeable.serializer(dataFrame)).expect("Failed to serialize frame"),
            frame.remote.address.port(),
            frame.remote.address.humanReadableAddress(),
        );
        return Ok(undefined);
    }

    onReceive(callback: (frame: ReceivedFrame) => void): void {
        this.#onReceive.listen(callback);
    }

    onClose(callback: () => void): void {
        this.#onClose.listen(callback);
        this.#abortController.abort();
    }

    close(): void {
        this.#socket.close();
    }
}
