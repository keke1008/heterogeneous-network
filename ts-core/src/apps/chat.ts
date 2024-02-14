import { CancelListening, EventBroker } from "@core/event";
import { BufferReader, BufferWriter, Destination, StreamService, StreamSocket, TunnelPortId } from "@core/net";
import { ConstantSerdeable, ObjectSerdeable, VariantSerdeable } from "@core/serde";
import { Utf8Serdeable } from "@core/serde/utf8";
import { Result, Ok, Err } from "oxide.ts";

const CHAT_PORT = TunnelPortId.schema.parse(103);

export interface TextMessage {
    type: "text";
    text: string;
}
const TextMessage = {
    serdeable: new ObjectSerdeable({ type: new ConstantSerdeable("text"), text: new Utf8Serdeable() }),
};

export interface AiImageMessage {
    type: "ai-image";
    prompt: string;
}

export type ChatMessage = TextMessage | AiImageMessage;
const typeToIndex = (type: ChatMessage["type"]): number => {
    const table = { text: 1, "ai-image": 3 } as const;
    return table[type];
};
const ChatMessage = {
    serdeable: new VariantSerdeable(
        [
            new ObjectSerdeable({ type: new ConstantSerdeable("text"), text: new Utf8Serdeable() }),
            new ObjectSerdeable({ type: new ConstantSerdeable("ai-image"), prompt: new Utf8Serdeable() }),
        ],
        (message: ChatMessage) => typeToIndex(message.type),
    ),
};

export class ChatClient {
    readonly id: symbol = Symbol();
    #socket: StreamSocket;

    private constructor(socket: StreamSocket) {
        this.#socket = socket;
    }

    static async connect(args: {
        service: StreamService;
        destination: Destination;
    }): Promise<Result<ChatClient, string>> {
        const socket = await args.service.connect({
            destination: args.destination,
            destinationPortId: CHAT_PORT,
        });
        if (socket.isErr()) {
            return socket;
        }

        return Ok(new ChatClient(socket.unwrap()));
    }

    static async accept(socket: StreamSocket): Promise<Result<ChatClient, string>> {
        return Ok(new ChatClient(socket));
    }

    async send(message: ChatMessage): Promise<Result<void, string>> {
        const buffer = BufferWriter.serialize(ChatMessage.serdeable.serializer(message));
        if (buffer.isErr()) {
            return Err(buffer.unwrapErr().name);
        }
        return this.#socket.send(buffer.unwrap());
    }

    onReceive(callback: (message: ChatMessage) => void): CancelListening {
        return this.#socket.onReceive((bytes) => {
            const result = BufferReader.deserialize(ChatMessage.serdeable.deserializer(), bytes);
            if (result.isOk()) {
                callback(result.unwrap());
            }
        });
    }

    async close(): Promise<void> {
        await this.#socket.close();
    }

    onClosed(callback: () => void): CancelListening {
        return this.#socket.onClose(callback);
    }
}

export class ChatServer {
    #service: StreamService;
    #cancelListening?: CancelListening;
    #onConnected = new EventBroker<ChatClient>();

    constructor(service: StreamService) {
        this.#service = service;
    }

    start(): Result<void, "already opened"> {
        const result = this.#service.listen(CHAT_PORT, (socket) => {
            console.log("ChatServer accepted", socket);
            socket.onReceive(async (data) => {
                console.log("ChatServer received", data);
                const result = await socket.send(data);
                if (result.isErr()) {
                    socket.close();
                }
            });
        });
        if (result.isErr()) {
            return result;
        }

        this.#cancelListening = result.unwrap();
        return Ok(undefined);
    }

    onConnected(callback: (client: ChatClient) => void): CancelListening {
        return this.#onConnected.listen(callback);
    }

    close() {
        this.#cancelListening?.();
    }

    [Symbol.dispose]() {
        this.close();
    }
}

class ChatClients {
    #clients = new Map<symbol, ChatClient>();
    #onAdd = new EventBroker<ChatClient>();

    add(client: ChatClient) {
        this.#clients.set(client.id, client);
        client.onClosed(() => {
            this.#clients.delete(client.id);
        });
    }

    onAdd(callback: (client: ChatClient) => void): CancelListening {
        return this.#onAdd.listen(callback);
    }

    close() {
        for (const client of this.#clients.values()) {
            client.close();
        }
    }
}

export class ChatApp {
    #service: StreamService;
    #server: ChatServer;
    #clients = new ChatClients();

    #onPeerConnected = new EventBroker<ChatClient>();

    constructor(service: StreamService) {
        this.#service = service;
        this.#server = new ChatServer(service);
        this.#server.start().unwrap();
        this.#server.onConnected((client) => {
            this.#clients.add(client);
        });

        this.#clients.onAdd((client) => {
            this.#onPeerConnected.emit(client);
        });
    }

    onPeerConnected(callback: (client: ChatClient) => void): CancelListening {
        return this.#onPeerConnected.listen(callback);
    }

    async connect(destination: Destination): Promise<Result<ChatClient, string>> {
        const client = await ChatClient.connect({ service: this.#service, destination });
        if (client.isOk()) {
            this.#clients.add(client.unwrap());
        }
        return client;
    }

    async close() {
        this.#server.close();
        this.#clients.close();
    }
}
