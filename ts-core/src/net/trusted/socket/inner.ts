import { BufferWriter } from "@core/net/buffer";
import { Destination } from "@core/net/node";
import { TunnelSocket, TunnelPortId } from "@core/net/tunnel";
import { Result } from "oxide.ts";
import { TrustedFrame, LengthOmittedPseudoHeader, ReceivedTrustedFrame, TrustedDataFrame } from "../frame";
import { IReceiver } from "@core/channel";
import { LocalNodeService } from "@core/net/local";

export class InnerSocket {
    #localNodeService: LocalNodeService;
    #socket: TunnelSocket;

    constructor(args: { localNodeService: LocalNodeService; socket: TunnelSocket }) {
        this.#localNodeService = args.localNodeService;
        this.#socket = args.socket;
    }

    get localPortId(): TunnelPortId {
        return this.#socket.localPortId;
    }

    get destination(): Destination {
        return this.#socket.destination;
    }

    get destinationPortId(): TunnelPortId {
        return this.#socket.destinationPortId;
    }

    async maxPayloadLength(): Promise<number> {
        const total = await this.#socket.maxPayloadLength();
        return total - TrustedDataFrame.headerLength();
    }

    async send(frame: TrustedFrame): Promise<Result<void, "timeout">> {
        const data = BufferWriter.serialize(TrustedFrame.serdeable.serializer(frame)).unwrap();
        const result = await this.#socket.send(data);
        return result.mapErr(() => "timeout");
    }

    receiver(): IReceiver<ReceivedTrustedFrame> {
        return this.#socket.receiver().filterMap((tunnelFrame) => {
            const deserializeResult = ReceivedTrustedFrame.deserialize(tunnelFrame);
            if (deserializeResult.isErr()) {
                console.warn("failed to deserialize tunnel frame", deserializeResult.unwrapErr());
                return undefined;
            }

            const frame = deserializeResult.unwrap();
            if (!frame.verifyChecksum()) {
                console.warn("checksum mismatch", frame);
                return undefined;
            }

            return frame;
        });
    }

    close(): void {
        this.#socket.close();
    }

    async createPseudoHeader(): Promise<LengthOmittedPseudoHeader> {
        return {
            source: await this.#localNodeService.getSource(),
            sourcePort: this.#socket.localPortId,
            destination: this.#socket.destination,
            destinationPortId: this.#socket.destinationPortId,
        };
    }
}
