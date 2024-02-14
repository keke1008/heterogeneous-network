import { CancelListening, EventBroker } from "@core/event";
import { BufferReader, BufferWriter, Destination, TrustedService, TrustedSocket, TunnelPortId } from "@core/net";
import { ObjectSerdeable, TransformSerdeable } from "@core/serde";
import { Utf8Serdeable } from "@core/serde/utf8";
import { Err, Ok, Result } from "oxide.ts";
import { z } from "zod";

export const AI_IMAGE_PORT = TunnelPortId.schema.parse(102);

export const aiImageHttpRequestBody = z.object({
    prompt: z.string(),
});
export type AiImageHttpRequestBody = z.infer<typeof aiImageHttpRequestBody>;
export const aiImageHttpResponseBody = z.object({ imageUrl: z.string() });
export type AiImageHttpResponseBody = z.infer<typeof aiImageHttpResponseBody>;

export const AI_IMAGE_HTTP_METHOD = "POST";
export const AI_IMAGE_HTTP_PATH = "/generate";
export const AI_IMAGE_HTTP_PORT = 14000;

export const fetchAiImageUrl = async (address: string, prompt: string): Promise<Result<string, unknown>> => {
    const param: AiImageHttpRequestBody = { prompt };
    const url = `http://${address}:${AI_IMAGE_HTTP_PORT}${AI_IMAGE_HTTP_PATH}`;
    return await fetch(url, { method: AI_IMAGE_HTTP_METHOD, body: JSON.stringify(param) })
        .then(async (res) => aiImageHttpResponseBody.parse(await res.json()))
        .then((res) => Ok(res.imageUrl))
        .catch((e) => Err(e));
};

export class AiImagePacket {
    httpServerAddress: string;
    prompt: string;

    constructor(args: { httpServerAddress: string; prompt: string }) {
        this.httpServerAddress = args.httpServerAddress;
        this.prompt = args.prompt;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            httpServerAddress: new Utf8Serdeable(),
            prompt: new Utf8Serdeable(),
        }),
        (obj) => new AiImagePacket(obj),
        (packet) => packet,
    );
}

export class AiImageServer {
    #service: TrustedService;
    #onReceive = new EventBroker<AiImagePacket>();
    #cancelListening: CancelListening | undefined;

    constructor(service: TrustedService) {
        this.#service = service;
    }

    onReceive(callback: (packet: AiImagePacket) => void): CancelListening {
        return this.#onReceive.listen(callback);
    }

    start(): Result<void, "already opened"> {
        const result = this.#service.listen(AI_IMAGE_PORT, (socket) => {
            socket.onReceive((data) => {
                const packet = BufferReader.deserialize(AiImagePacket.serdeable.deserializer(), data);
                if (packet.isOk()) {
                    this.#onReceive.emit(packet.unwrap());
                }
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

export class AiImageClient {
    #socket: TrustedSocket;

    constructor(socket: TrustedSocket) {
        this.#socket = socket;
    }

    static async connect(args: {
        trustedService: TrustedService;
        destination: Destination;
    }): Promise<Result<AiImageClient, "timeout" | "already opened">> {
        const socket = await args.trustedService.connect({
            destination: args.destination,
            destinationPortId: AI_IMAGE_PORT,
        });
        return socket.map((socket) => new AiImageClient(socket));
    }

    async send(params: AiImagePacket): Promise<Result<void, "timeout" | "invalid operation">> {
        const buf = BufferWriter.serialize(AiImagePacket.serdeable.serializer(params)).unwrap();
        return await this.#socket.send(buf);
    }

    async close() {
        await this.#socket.close();
    }

    onClose(callback: () => void) {
        return this.#socket.onClose(callback);
    }
}
