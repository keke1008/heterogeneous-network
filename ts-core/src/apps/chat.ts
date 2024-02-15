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
    imageUrl?: Promise<string | undefined>;
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
            const table = { text: 1, "ai-image": 2 } as const;
            return table[message.type];
        },
    ),
};

export interface Message {
    uuid: string;
    sender: MessageSender;
    sentAt: Date;
    data: MessageData;
}

type MessagePacket = Omit<Message, "uuid" | "sender">;
const MessagePacket = {
    serdeable: new ObjectSerdeable<MessagePacket>({
        sentAt: new DateSerdeable(),
        data: MessageData.serdeable,
    }),
};

class ChatClient {
    readonly uuid: string = crypto.randomUUID();
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

    static accept(socket: StreamSocket): ChatClient {
        return new ChatClient(socket);
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
            const client = ChatClient.accept(socket);
            this.#onConnected.emit(client);
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

export class ChatRoom {
    #client: ChatClient;
    #history: Message[] = [];
    #onMessage = new EventBroker<Message>();

    constructor(client: ChatClient) {
        this.#client = client;
        this.#client.onReceive((packet) => {
            const message: Message = { uuid: crypto.randomUUID(), sender: "peer", ...packet };
            this.#history.push(message);
            this.#onMessage.emit(message);
        });
    }

    get uuid() {
        return this.#client.uuid;
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

        const sentMessage: Message = { uuid: crypto.randomUUID(), sender: "self", ...message };
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

    onMessage(callback: (message: Message) => void): CancelListening {
        return this.#onMessage.listen(callback);
    }

    onClose(callback: () => void): CancelListening {
        return this.#client.onClose(callback);
    }

    async close() {
        await this.#client.close();
    }
}

class ChatRooms {
    #rooms = new Map<string, ChatRoom>();
    #onAdd = new EventBroker<ChatRoom>();
    #onClose = new EventBroker<ChatRoom>();

    add(client: ChatClient): ChatRoom {
        const room = new ChatRoom(client);
        this.#rooms.set(client.uuid, room);
        this.#onAdd.emit(room);

        client.onClose(() => {
            this.#rooms.delete(client.uuid);
            this.#onClose.emit(room);
        });

        return room;
    }

    onAdd(callback: (room: ChatRoom) => void): CancelListening {
        return this.#onAdd.listen(callback);
    }

    onClose(callback: (room: ChatRoom) => void): CancelListening {
        return this.#onClose.listen(callback);
    }

    closeAll() {
        for (const client of this.#rooms.values()) {
            client.close();
        }
    }

    rooms(): ChatRoom[] {
        return [...this.#rooms.values()];
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

    onRoomClosed(callback: (room: ChatRoom) => void): CancelListening {
        return this.#rooms.onClose(callback);
    }

    async connect(destination: Destination): Promise<Result<ChatRoom, string>> {
        const client = await ChatClient.connect({ service: this.#service, destination });
        if (client.isOk()) {
            const room = this.#rooms.add(client.unwrap());
            return Ok(room);
        }
        return Err(client.unwrapErr());
    }

    rooms(): ChatRoom[] {
        return this.#rooms.rooms();
    }

    async close() {
        this.#server.close();
        this.#rooms.closeAll();
    }
}
