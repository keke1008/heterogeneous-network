import { Handle, sleep, spawn, withCancel } from "@core/async";
import { BufferWriter } from "@core/net/buffer";
import { LocalNodeService } from "@core/net/local";
import { Destination } from "@core/net/node";
import { TunnelSocket, TunnelPortId } from "@core/net/tunnel";
import { Duration } from "@core/time";
import { Result, Err, Ok } from "oxide.ts";
import {
    TrustedFrame,
    ReceivedTrustedFrame,
    LengthOmittedPseudoHeader,
    TrustedFrameBody,
    TrustedRequestFrameBody,
} from "../frame";
import { IReceiver, Sender } from "@core/channel";
import { deferred } from "@core/deferred";

class TunnelSocketWrapper {
    #socket: TunnelSocket;

    constructor(socket: TunnelSocket) {
        this.#socket = socket;
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
}

class SendFrameThreadHandle {
    #sender = new Sender<[TrustedFrame, (result: Result<void, "timeout">) => void]>();
    #handle: Handle<void>;

    constructor(args: { socket: TunnelSocketWrapper; retryCount: number; retryInterval: Duration }) {
        this.#handle = spawn(async (signal) => {
            signal.addEventListener("abort", () => this.#sender.close());

            outer: for await (const [frame, result] of this.#sender.receiver()) {
                for (let i = 0; i < args.retryCount; i++) {
                    if (i > 0) {
                        await sleep(args.retryInterval);
                    }

                    const sendResult = await args.socket.send(frame);
                    if (sendResult.isOk()) {
                        result(Ok(undefined));
                        continue outer;
                    }
                }

                result(Err("timeout"));
            }
        });
    }

    send(frame: TrustedFrame): Promise<Result<void, "timeout">> {
        return new Promise((resolve) => this.#sender.send([frame, resolve]));
    }

    async close(): Promise<void> {
        this.#handle.cancel();
        await this.#handle;
    }
}

export class InnerSocket {
    #localNodeService: LocalNodeService;
    #socket: TunnelSocketWrapper;
    #sendFrameThreadHandle: SendFrameThreadHandle;

    #sendRetryCount: number = 3;
    #timeoutInterval: Duration = Duration.fromMillies(1000);

    constructor(args: { localNodeService: LocalNodeService; socket: TunnelSocket }) {
        this.#localNodeService = args.localNodeService;
        this.#socket = new TunnelSocketWrapper(args.socket);
        this.#sendFrameThreadHandle = new SendFrameThreadHandle({
            socket: this.#socket,
            retryCount: this.#sendRetryCount,
            retryInterval: this.#timeoutInterval,
        });
    }

    receiver(): IReceiver<ReceivedTrustedFrame> {
        return this.#socket.receiver();
    }

    get sendRetryCount(): number {
        return this.#sendRetryCount;
    }

    get timeoutInterval(): Duration {
        return this.#timeoutInterval;
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

    async #createPseudoHeader(): Promise<LengthOmittedPseudoHeader> {
        return {
            source: await this.#localNodeService.getSource(),
            sourcePort: this.#socket.localPortId,
            destination: this.#socket.destination,
            destinationPortId: this.#socket.destinationPortId,
        };
    }

    async sendAndReceiveAck(body: TrustedRequestFrameBody): Promise<Result<void, "timeout">> {
        const frame = TrustedFrame.create({ body, pseudoHeader: await this.#createPseudoHeader() });
        for (let i = 0; i < this.#sendRetryCount; i++) {
            if (i > 0) {
                sleep(this.#timeoutInterval);
            }

            const sendResult = await this.#sendFrameThreadHandle.send(frame);
            if (sendResult.isErr()) {
                continue;
            }

            const controller = new AbortController();
            sleep(this.#timeoutInterval).then(() => controller.abort());
            const result = deferred<Result<void, "timeout">>();
            result.then(() => controller.abort());

            const ack = await withCancel({
                signal: controller.signal,
                promise: this.receiver()
                    .filter((frame) => body.isCorrespondingAckFrame(frame.body))
                    .next(controller.signal),
                onCancel: () => ({ value: undefined }),
            });
            if (ack.value !== undefined) {
                return Ok(undefined);
            }
        }

        return Err("timeout");
    }

    async send(frame: TrustedFrame | TrustedFrameBody): Promise<Result<void, "timeout">> {
        if (frame instanceof TrustedFrame) {
            return this.#sendFrameThreadHandle.send(frame);
        } else {
            return this.#socket.send(
                TrustedFrame.create({
                    body: frame,
                    pseudoHeader: await this.#createPseudoHeader(),
                }),
            );
        }
    }

    async close(): Promise<void> {
        await this.#sendFrameThreadHandle.close();
        this.#socket.close();
    }
}
