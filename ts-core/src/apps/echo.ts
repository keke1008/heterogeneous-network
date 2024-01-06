import { CancelListening } from "@core/event";
import { Destination, TrustedService, TrustedSocket, TunnelPortId } from "@core/net";
import { Ok, Result } from "oxide.ts";

const ECHO_PORT = TunnelPortId.schema.parse(100);

export class EchoServer {
    #service: TrustedService;
    #cancelListening?: CancelListening;

    constructor(args: { trustedService: TrustedService }) {
        this.#service = args.trustedService;
    }

    start(): Result<void, "already opened"> {
        const result = this.#service.listen(ECHO_PORT, (socket) => {
            socket.onReceive(async (data) => {
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

    close(): void {
        this.#cancelListening?.();
    }

    [Symbol.dispose](): void {
        this.close();
    }
}

export class EchoClient {
    #socket: TrustedSocket;

    private constructor(args: { socket: TrustedSocket }) {
        this.#socket = args.socket;
    }

    static async connect(args: {
        trustedService: TrustedService;
        destination: Destination;
    }): Promise<Result<EchoClient, "timeout" | "already opened">> {
        const socket = await args.trustedService.connect({
            destination: args.destination,
            destinationPortId: ECHO_PORT,
        });
        return socket.map((socket) => new EchoClient({ socket }));
    }

    async send(message: string): Promise<Result<void, "timeout">> {
        const bytes = new TextEncoder().encode(message);
        return this.#socket.send(bytes);
    }

    onReceive(callback: (message: string) => void): CancelListening {
        return this.#socket.onReceive((bytes) => {
            const str = new TextDecoder().decode(bytes);
            callback(str);
        });
    }

    async close(): Promise<void> {
        await this.#socket.close();
    }
}
