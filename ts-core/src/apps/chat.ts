import { CancelListening, EventBroker } from "@core/event";
import { BufferReader, BufferWriter, Destination, StreamService, StreamSocket, TunnelPortId } from "@core/net";
import {
    ConstantSerdeable,
    DateSerdeable,
    ObjectSerdeable,
    Uint32Serdeable,
    Utf8Serdeable,
    VariantSerdeable,
} from "@core/serde";
import { Result, Ok, Err } from "oxide.ts";

const CHAT_PORT = TunnelPortId.schema.parse(103);

export type MessageSender = "self" | "peer";

export interface TextMessageData {
    type: "text";
    text: string;
}

export interface AiImageMessageData {
    type: "ai-image";
    prompt: string;
    imageUrl?: Promise<string>;
}
export type MessageData = TextMessageData | AiImageMessageData;
const MessageData = {
    serdeable: new VariantSerdeable(
        [
            new ObjectSerdeable({
                type: new ConstantSerdeable("text"),
                text: new Utf8Serdeable(new Uint32Serdeable()),
            }),
            new ObjectSerdeable({
                type: new ConstantSerdeable("ai-image"),
                prompt: new Utf8Serdeable(new Uint32Serdeable()),
            }),
        ],
        (message: MessageData) => {
            const table = { text: 1, "ai-image": 3 } as const;
            return table[message.type];
        },
    ),
};

export interface Message {
    id: symbol;
    sender: MessageSender;
    sentAt: Date;
    data: MessageData;
}

type MessagePacket = Omit<Message, "id" | "sender">;
const MessagePacket = {
    serdeable: new ObjectSerdeable<MessagePacket>({
        sentAt: new DateSerdeable(),
        data: MessageData.serdeable,
    }),
};

class ChatClient {
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

    get remote(): Destination {
        return this.#socket.destination;
    }

    get remotePortId(): TunnelPortId {
        return this.#socket.destinationPortId;
    }

    async send(message: MessagePacket): Promise<Result<void, string>> {
        const buffer = BufferWriter.serialize(MessagePacket.serdeable.serializer(message));
        if (buffer.isErr()) {
            return Err(buffer.unwrapErr().name);
        }
        return this.#socket.send(buffer.unwrap());
    }

    onReceive(callback: (message: MessagePacket) => void): CancelListening {
        return this.#socket.onReceive((bytes) => {
            const result = BufferReader.deserialize(MessagePacket.serdeable.deserializer(), bytes);
            if (result.isOk()) {
                callback(result.unwrap());
            }
        });
    }

    async close(): Promise<void> {
        await this.#socket.close();
    }

    onClose(callback: () => void): CancelListening {
        return this.#socket.onClose(callback);
    }
}

class ChatServer {
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

class ChatRoom {
    #client: ChatClient;
    #history: Message[] = [];
    #onMessage = new EventBroker<Message>();

    constructor(client: ChatClient) {
        this.#client = client;
        this.#client.onReceive((packet) => {
            const message: Message = { id: Symbol(), sender: "peer", ...packet };
            this.#history.push(message);
            this.#onMessage.emit(message);
        });
    }

    get id() {
        return this.#client.id;
    }

    get history(): readonly Readonly<Message>[] {
        return this.#history;
    }

    get remote(): Destination {
        return this.#client.remote;
    }

    get remotePortId(): TunnelPortId {
        return this.#client.remotePortId;
    }

    async #sendMessage(message: MessagePacket): Promise<Result<void, string>> {
        const sendResult = await this.#client.send(message);
        if (sendResult.isErr()) {
            return sendResult;
        }

        const sentMessage: Message = { id: Symbol(), sender: "self", ...message };
        this.#history.push(sentMessage);
        this.#onMessage.emit(sentMessage);
        return Ok(undefined);
    }

    async sendTextMessage(text: string): Promise<Result<void, string>> {
        return this.#sendMessage({
            sentAt: new Date(),
            data: { type: "text", text },
        });
    }

    async sendAiImageMessage(prompt: string): Promise<Result<void, string>> {
        return this.#sendMessage({
            sentAt: new Date(),
            data: { type: "ai-image", prompt },
        });
    }

    updateAiImageMessage(id: symbol, imageUrl: Promise<string>) {
        const message = this.#history.find((m) => m.id === id);
        if (message?.data.type === "ai-image") {
            message.data.imageUrl = imageUrl;
        }
    }

    onMessage(callback: (message: Message) => void): CancelListening {
        return this.#onMessage.listen(callback);
    }

    onClose(callback: () => void): CancelListening {
        return this.#client.onClose(callback);
    }

    close() {
        this.#client.close();
    }
}

class ChatRooms {
    #rooms = new Map<symbol, ChatRoom>();
    #onAdd = new EventBroker<ChatRoom>();

    add(client: ChatClient) {
        const room = new ChatRoom(client);
        this.#rooms.set(client.id, room);
        this.#onAdd.emit(room);

        client.onClose(() => {
            this.#rooms.delete(client.id);
        });
    }

    onAdd(callback: (room: ChatRoom) => void): CancelListening {
        return this.#onAdd.listen(callback);
    }

    closeAll() {
        for (const client of this.#rooms.values()) {
            client.close();
        }
    }
}

export class ChatApp {
    #service: StreamService;
    #server: ChatServer;
    #rooms = new ChatRooms();

    constructor(service: StreamService) {
        this.#service = service;
        this.#server = new ChatServer(service);
        this.#server.start().unwrap();
        this.#server.onConnected((client) => {
            this.#rooms.add(client);
        });
    }

    onRoomCreated(callback: (room: ChatRoom) => void): CancelListening {
        return this.#rooms.onAdd(callback);
    }

    async connect(destination: Destination): Promise<Result<ChatClient, string>> {
        const client = await ChatClient.connect({ service: this.#service, destination });
        if (client.isOk()) {
            this.#rooms.add(client.unwrap());
        }
        return client;
    }

    async close() {
        this.#server.close();
        this.#rooms.closeAll();
    }
}
