import { CancelListening, EventBroker } from "@core/event";
import { BufferReader, TunnelPortId, TunnelService } from "@core/net";
import { TransformSerdeable, ObjectSerdeable, Utf8Serdeable } from "@core/serde";
import { Result, Ok } from "oxide.ts";

export const POSTING_PORT = TunnelPortId.schema.parse(104);

export class PostingPacket {
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
