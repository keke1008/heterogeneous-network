import { SingleListenerEventBroker } from "@core/event";
import { Result } from "oxide.ts";
import * as ws from "ws";
import { RequestMessage, ResponseMessage } from "./message";
import { BufferReader, BufferWriter } from "@core/net";

export class IpcSocket {
    #socket: ws.WebSocket;
    #message = new SingleListenerEventBroker<RequestMessage>();

    private constructor(socket: ws.WebSocket) {
        this.#socket = socket;
        socket.on("message", (data) => {
            if (!(data instanceof Buffer)) {
                console.warn("Received non-buffer data from socket");
                return;
            }

            const message = BufferReader.deserialize(RequestMessage.serdeable.deserializer(), data);
            if (message.isErr()) {
                console.warn("Failed to deserialize message", message.unwrapErr());
                return;
            }

            this.#message.emit(message.unwrap());
        });
    }

    static async connect(port: number): Promise<Result<IpcSocket, Error>> {
        const socket = new ws.WebSocket(`ws://localhost:${port}`);
        return await Result.safe(
            new Promise<IpcSocket>((resolve, reject) => {
                socket.once("open", () => resolve(new IpcSocket(socket)));
                socket.once("error", reject);
            }),
        );
    }

    send(message: ResponseMessage): void {
        const serialized = BufferWriter.serialize(ResponseMessage.serdeable.serializer(message)).unwrap();
        this.#socket.send(serialized);
    }

    onMessage(callback: (message: RequestMessage) => void): void {
        this.#message.listen(callback);
    }

    close(): void {
        this.#socket.close();
    }
}
