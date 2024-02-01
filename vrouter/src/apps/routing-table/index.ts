import { CancelListening, EventBroker } from "@core/event";
import { BufferReader, BufferWriter, Destination, StreamService, StreamSocket, TunnelPortId } from "@core/net";
import { Matcher, RoutingEntry, StaticRoutingService } from "@vrouter/routing";
import { RequestPacket, ResponsePacket } from "./packet";
import { P, match } from "ts-pattern";

export const ROUTING_TABLE_PORT = TunnelPortId.schema.parse(200);

export class RoutingTableServer {
    #service: StaticRoutingService;
    #cancelListening?: CancelListening;

    constructor(args: { routingService: StaticRoutingService }) {
        this.#service = args.routingService;
    }

    start({ streamService }: { streamService: StreamService }) {
        const result = streamService.listen(ROUTING_TABLE_PORT, (socket) => {
            const sendEntries = async () => {
                const response = new ResponsePacket.Entries(this.#service.getEntries());
                const buffer = BufferWriter.serialize(ResponsePacket.serdeable.serializer(response)).unwrap();
                const sendResult = await socket.send(buffer);
                if (sendResult.isErr()) {
                    console.warn("Failed to send response", sendResult.unwrapErr());
                }
            };

            sendEntries();

            socket.onReceive(async (data) => {
                const packetResult = BufferReader.deserialize(RequestPacket.serdeable.deserializer(), data);
                if (packetResult.isErr()) {
                    console.warn("Failed to deserialize packet", packetResult.unwrapErr());
                    return;
                }

                const packet = packetResult.unwrap();
                match(packet)
                    .with(P.instanceOf(RequestPacket.RequestEntries), () => {})
                    .with(P.instanceOf(RequestPacket.UpdateEntries), (p) => {
                        for (const entry of p.entries) {
                            this.#service.updateEntry(entry);
                        }
                    })
                    .with(P.instanceOf(RequestPacket.DeleteEntries), (p) => {
                        for (const matcher of p.matchers) {
                            this.#service.deleteEntry(matcher);
                        }
                    })
                    .exhaustive();

                await sendEntries();
            });
        });

        if (result.isOk()) {
            this.#cancelListening = result.unwrap();
        } else {
            console.warn("Failed to start listening", result.unwrapErr());
        }
        return result;
    }

    close() {
        this.#cancelListening?.();
    }

    [Symbol.dispose]() {
        this.close();
    }
}

export class RoutingTableClient {
    #socket: StreamSocket;
    #onUpdateEntries = new EventBroker<RoutingEntry[]>();

    private constructor(socket: StreamSocket) {
        this.#socket = socket;
        socket.onReceive(async (data) => {
            const packetResult = BufferReader.deserialize(ResponsePacket.serdeable.deserializer(), data);
            if (packetResult.isErr()) {
                console.warn("Failed to deserialize packet", packetResult.unwrapErr());
                return;
            }

            const packet = packetResult.unwrap();
            match(packet)
                .with(P.instanceOf(ResponsePacket.Entries), (p) => {
                    this.#onUpdateEntries.emit(p.entries);
                })
                .exhaustive();
        });
    }

    static async connect(args: { streamService: StreamService; destination: Destination }) {
        const result = await args.streamService.connect({
            destination: args.destination,
            destinationPortId: ROUTING_TABLE_PORT,
        });
        return result.map((socket) => new RoutingTableClient(socket));
    }

    onUpdateEntries(callback: (entries: RoutingEntry[]) => void) {
        return this.#onUpdateEntries.listen(callback);
    }

    async requestEntries(): Promise<void> {
        const request = new RequestPacket.RequestEntries();
        const buffer = BufferWriter.serialize(RequestPacket.serdeable.serializer(request)).unwrap();
        await this.#socket.send(buffer);
    }

    async updateEntries(entries: RoutingEntry[]): Promise<void> {
        const request = new RequestPacket.UpdateEntries(entries);
        const buffer = BufferWriter.serialize(RequestPacket.serdeable.serializer(request)).unwrap();
        await this.#socket.send(buffer);
    }

    async deleteEntries(matchers: Matcher[]): Promise<void> {
        const request = new RequestPacket.DeleteEntries(matchers);
        const buffer = BufferWriter.serialize(RequestPacket.serdeable.serializer(request)).unwrap();
        await this.#socket.send(buffer);
    }
}
