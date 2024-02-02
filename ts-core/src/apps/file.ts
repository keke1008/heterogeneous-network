import { CancelListening } from "@core/event";
import { BufferReader, BufferWriter, Destination, StreamService, StreamSocket, TunnelPortId } from "@core/net";
import { ObjectSerdeable, RemainingBytesSerdeable, TransformSerdeable } from "@core/serde";
import { Utf8Serdeable } from "@core/serde/utf8";
import { Ok, Result } from "oxide.ts";

const FILE_PORT = TunnelPortId.schema.parse(101);

class Packet {
    filename: string;
    data: Uint8Array;

    constructor(args: { filename: string; data: Uint8Array }) {
        this.filename = args.filename;
        this.data = args.data;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            filename: Utf8Serdeable,
            data: new RemainingBytesSerdeable(),
        }),
        (obj) => new Packet(obj),
        (packet) => packet,
    );
}

const downloadAsFile = (data: Uint8Array, filename: string) => {
    const file = new File([data], filename);
    const url = URL.createObjectURL(file);

    const a = document.createElement("a");
    a.href = url;
    a.download = filename;
    a.click();
    a.remove();
    URL.revokeObjectURL(url);
};

const onReceivePacketData = async (data: Uint8Array) => {
    const result = BufferReader.deserialize(Packet.serdeable.deserializer(), data);
    if (result.isErr()) {
        console.error("failed to deserialize packet", result.unwrapErr());
        return;
    }

    const packet = result.unwrap();
    downloadAsFile(packet.data, packet.filename);
};

export class FileServer {
    #service: StreamService;
    #cancelListening?: CancelListening;

    constructor(args: { service: StreamService }) {
        this.#service = args.service;
    }

    start(): Result<void, "already opened"> {
        const result = this.#service.listen(FILE_PORT, (socket) => {
            socket.onReceive(async (data) => {
                onReceivePacketData(data);
            });
        });
        if (result.isErr()) {
            return result;
        }

        this.#cancelListening = result.unwrap();
        return Ok(undefined);
    }

    close(): void {
        this.#cancelListening?.();
    }

    [Symbol.dispose](): void {
        this.close();
    }
}

export class FileClient {
    #socket: StreamSocket;

    private constructor(args: { socket: StreamSocket }) {
        this.#socket = args.socket;
    }

    static async connect(args: {
        streamService: StreamService;
        destination: Destination;
    }): Promise<Result<FileClient, string>> {
        const socketResult = await args.streamService.connect({
            destination: args.destination,
            destinationPortId: FILE_PORT,
        });
        if (socketResult.isErr()) {
            return socketResult;
        }

        const socket = socketResult.unwrap();
        socket.onReceive(async (data) => {
            onReceivePacketData(data);
        });

        return Ok(new FileClient({ socket: socketResult.unwrap() }));
    }

    async sendFile(args: { filename: string; data: Uint8Array }): Promise<Result<void, string>> {
        const packet = new Packet(args);
        const data = BufferWriter.serialize(Packet.serdeable.serializer(packet)).unwrap();
        return await this.#socket.send(data);
    }

    async close(): Promise<void> {
        await this.#socket.close();
    }

    async [Symbol.asyncDispose](): Promise<void> {
        await this.close();
    }

    onClose(callback: () => void): CancelListening {
        return this.#socket.onClose(callback);
    }
}
