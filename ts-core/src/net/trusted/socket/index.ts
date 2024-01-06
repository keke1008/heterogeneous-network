import { LocalNodeService } from "@core/net/local";
import { TunnelSocket } from "@core/net/tunnel";
import { Result, Err, Ok } from "oxide.ts";
import { InnerSocket } from "./inner";
import { DataFrameBody, FinAckFrameBody, FinFrameBody, SynAckFrameBody, SynFrameBody } from "../frame";
import { CancelListening, SingleListenerEventBroker } from "@core/event";
import { sleep, spawn, withCancel } from "@core/async";
import { SequenceNumber } from "../sequenceNumber";

export type TrustedSocketState = "CLOSED" | "CLOSING" | "ESTABLISHED";

export class IllegalStateError extends Error {}

export class TrustedSocket {
    #socket: InnerSocket;
    #onReceiveData = new SingleListenerEventBroker<Uint8Array>();
    #state: TrustedSocketState = "ESTABLISHED";
    #nextSequenceNumber = SequenceNumber.zero();

    static async activeOpen(args: {
        localNodeService: LocalNodeService;
        socket: TunnelSocket;
    }): Promise<Result<TrustedSocket, "timeout">> {
        const socket = new InnerSocket({ localNodeService: args.localNodeService, socket: args.socket });

        const ackReplied = spawn(async (signal) => {
            while (!signal.aborted) {
                const syn = await withCancel({
                    signal,
                    onCancel: () => ({ value: undefined }),
                    promise: socket
                        .receiver()
                        .filter((frame) => frame.body instanceof SynFrameBody)
                        .next(),
                });
                if (syn.value === undefined) {
                    return false;
                }

                const ackReplyResut = await socket.send(new SynAckFrameBody());
                if (ackReplyResut.isOk()) {
                    return true;
                }
            }

            return false;
        });

        const synResult = await socket.sendAndReceiveAck(new SynFrameBody());
        if (synResult.isErr()) {
            ackReplied.cancel();
            return synResult;
        }

        sleep(socket.timeoutInterval).then(() => ackReplied.cancel());
        if (await ackReplied) {
            return Ok(new TrustedSocket(socket));
        }

        return Err("timeout");
    }

    static async passiveOpen(args: {
        localNodeService: LocalNodeService;
        socket: TunnelSocket;
    }): Promise<Result<TrustedSocket, "timeout" | "illegal frame">> {
        const socket = new InnerSocket({ localNodeService: args.localNodeService, socket: args.socket });
        const firstFrame = await socket.receiver().next();
        if (firstFrame.done) {
            return Err("illegal frame");
        }

        if (!(firstFrame.value.body instanceof SynFrameBody)) {
            return Err("illegal frame");
        }

        const replyAck = await socket.send(new SynAckFrameBody());
        if (replyAck.isErr()) {
            return replyAck;
        }

        const synResult = await socket.sendAndReceiveAck(new SynFrameBody());
        if (synResult.isErr()) {
            return synResult;
        }

        return Ok(new TrustedSocket(socket));
    }

    private constructor(socket: InnerSocket) {
        this.#socket = socket;
        this.#socket.receiver().forEach(async (frame) => {
            if (frame.body instanceof DataFrameBody) {
                this.#onReceiveData.emit(frame.body.payload);
                this.#socket.send(frame.body.createAckFrameBody());
                return;
            }

            if (frame.body instanceof FinFrameBody) {
                this.#passiveClose();
                return;
            }
        });
    }

    async send(data: Uint8Array): Promise<Result<void, "timeout">> {
        if (this.#state !== "ESTABLISHED") {
            throw new IllegalStateError("socket is not established");
        }

        const result = await this.#socket.sendAndReceiveAck(new DataFrameBody(this.#nextSequenceNumber, data));
        this.#nextSequenceNumber = this.#nextSequenceNumber.next();
        if (result.isErr()) {
            this.close();
        }
        return result;
    }

    onReceive(callback: (data: Uint8Array) => void): CancelListening {
        return this.#onReceiveData.listen(callback);
    }

    async #close(): Promise<void> {
        this.#state = "CLOSED";
        await this.#socket.close();
    }

    async #passiveClose(): Promise<void> {
        if (this.#state !== "ESTABLISHED") {
            return;
        }
        this.#state = "CLOSING";

        const replyFinAck = await this.#socket.send(new FinAckFrameBody());
        if (replyFinAck.isErr()) {
            await this.#close();
            return;
        }

        await this.#socket.sendAndReceiveAck(new FinFrameBody());
        await this.#close();
    }

    async #activeClose(): Promise<void> {
        if (this.#state !== "ESTABLISHED") {
            return;
        }
        this.#state = "CLOSING";

        const finAckReplied = spawn(async (signal) => {
            while (!signal.aborted) {
                const fin = await withCancel({
                    signal,
                    onCancel: () => ({ value: undefined }),
                    promise: this.#socket
                        .receiver()
                        .filter((frame) => frame.body instanceof FinFrameBody)
                        .next(),
                });
                if (fin.value === undefined) {
                    return false;
                }

                const ackReplyResut = await this.#socket.send(new FinAckFrameBody());
                if (ackReplyResut.isOk()) {
                    return true;
                }
            }

            return false;
        });

        const sendFinResult = await this.#socket.sendAndReceiveAck(new FinFrameBody());
        if (sendFinResult.isErr()) {
            finAckReplied.cancel();
            await this.#close();
            return;
        }

        sleep(this.#socket.timeoutInterval).then(() => finAckReplied.cancel());
        await finAckReplied;
        await this.#close();
    }

    async close(): Promise<void> {
        if (this.#state === "ESTABLISHED") {
            await this.#activeClose();
            await this.#close();
        }
    }

    [Symbol.asyncDispose](): Promise<void> {
        return this.close();
    }
}
