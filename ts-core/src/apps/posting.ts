import { CancelListening, EventBroker } from "@core/event";
import { BufferReader, BufferWriter, Destination, TunnelPortId, TunnelService } from "@core/net";
import { TransformSerdeable, ObjectSerdeable, Utf8Serdeable } from "@core/serde";
import { Result, Ok } from "oxide.ts";

const POSTING_PORT = TunnelPortId.schema.parse(104);

class PostingPacket {
    content: string;

    constructor(args: { content: string }) {
        this.content = args.content;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ content: new Utf8Serdeable() }),
        (obj) => new PostingPacket(obj),
        (packet) => packet,
    );
}

export class PostingServer {
    #service: TunnelService;
    #cancelListening?: CancelListening;
    #onReceive = new EventBroker<string>();

    constructor(args: { tunnelServer: TunnelService }) {
        this.#service = args.tunnelServer;
    }

    start(): Result<void, "already opened"> {
        const result = this.#service.listen(POSTING_PORT, (socket) => {
            console.log("EchoServer accepted", socket);
            socket.receiver().forEach((frame) => {
                const packet = BufferReader.deserialize(PostingPacket.serdeable.deserializer(), frame.data);
                if (packet.isErr()) {
                    console.warn("Failed to deserialize packet", packet.unwrapErr());
                } else {
                    this.#onReceive.emit(packet.unwrap().content);
                }
            });
        });
        if (result.isErr()) {
            return result;
        }

        this.#cancelListening = result.unwrap();
        return Ok(undefined);
    }

    onReceive(callback: (data: string) => void): CancelListening {
        return this.#onReceive.listen(callback);
    }

    close(): void {
        this.#cancelListening?.();
    }

    [Symbol.dispose](): void {
        this.close();
    }
}

export class PostingClient {
    #service: TunnelService;

    constructor(args: { service: TunnelService }) {
        this.#service = args.service;
    }

    async send(args: { destination: Destination; data: string }): Promise<Result<void, unknown>> {
        const socket = this.#service.openDynamicPort({
            destination: args.destination,
            destinationPortId: POSTING_PORT,
        });
        if (socket.isErr()) {
            return socket;
        }

        using s = socket.unwrap();
        const packet = new PostingPacket({ content: args.data });
        const buf = BufferWriter.serialize(PostingPacket.serdeable.serializer(packet)).unwrap();
        return await s.send(buf);
    }
}
