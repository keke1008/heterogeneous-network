import { Result, Err, Ok } from "oxide.ts";
import { InnerSocket } from "./inner";
import { DataFrameBody, ReceivedTrustedFrame, SynFrameBody, TrustedFrame } from "../frame";
import { CancelListening, SingleListenerEventBroker } from "@core/event";
import { SocketState, SocketAction } from "./state";
import { match } from "ts-pattern";
import { sleep } from "@core/async";
import { RETRY_INTERVAL } from "../constants";
import { TunnelSocket } from "@core/net/tunnel";
import { LocalNodeService } from "@core/net/local";

export type TrustedSocketState = "CLOSED" | "CLOSING" | "ESTABLISHED";

export class IllegalStateError extends Error {}

export class TrustedSocket {
    #state = new SocketState();
    #socket: InnerSocket;
    #onReceiveData = new SingleListenerEventBroker<Uint8Array>();
    #onOpen = new SingleListenerEventBroker<void>();
    #onClose = new SingleListenerEventBroker<void>();

    #processActions(actions: SocketAction[]) {
        for (const action of actions) {
            match(action)
                .with({ type: "open" }, () => {
                    this.#onOpen.emit();
                })
                .with({ type: "close" }, () => {
                    this.#onClose.emit();
                })
                .with({ type: "receive" }, (action) => {
                    this.#onReceiveData.emit(action.data);
                })
                .with({ type: "send" }, async (action) => {
                    const frame = TrustedFrame.create({
                        body: action.body,
                        pseudoHeader: await this.#socket.createPseudoHeader(),
                    });
                    const result = await this.#socket.send(frame);
                    const actions = result.isOk()
                        ? this.#state.onSendSuccess(action)
                        : this.#state.onSendFailure(action);
                    this.#processActions(actions);
                })
                .with({ type: "ack timeout" }, async (action) => {
                    await sleep(RETRY_INTERVAL);
                    this.#processActions(this.#state.onReceiveAckTimeout(action));
                })
                .with({ type: "delay" }, async (action) => {
                    await sleep(RETRY_INTERVAL);
                    this.#processActions(action.actions);
                })
                .exhaustive();
        }
    }

    private constructor(socket: TunnelSocket, localNodeService: LocalNodeService) {
        this.#socket = new InnerSocket({ socket, localNodeService });
        this.#socket.receiver().forEach((frame) => {
            this.#processActions(this.#state.onReceive(frame.body));
        });
    }

    static async #open(socket: TrustedSocket, actions: SocketAction[]): Promise<Result<TrustedSocket, "timeout">> {
        let cancelHandleOpen: (() => void) | undefined;
        const handleOpen = new Promise<Result<void, "timeout">>((resolve) => {
            cancelHandleOpen = socket.#state.onStateChange((state) => {
                state === "open" && resolve(Ok(undefined));
            });
        });

        let cancelHandleError: (() => void) | undefined;
        const handleError = new Promise<Result<void, "timeout">>((resolve) => {
            cancelHandleError = socket.#state.onError(() => resolve(Err("timeout")));
        });

        const connectResult = Promise.race([handleOpen, handleError]);
        connectResult.finally(() => {
            cancelHandleOpen?.();
            cancelHandleError?.();
        });

        socket.#processActions(actions);
        const result = await connectResult;
        return result.map(() => socket);
    }

    static async accept(
        tunnelSocket: TunnelSocket,
        localNodeService: LocalNodeService,
    ): Promise<Result<TrustedSocket, "timeout" | "invalid frame" | "checksum mismatch">> {
        const firstFrame = await tunnelSocket.receiver().next();
        if (firstFrame.done) {
            return Err("timeout");
        }

        const receivedFrame = ReceivedTrustedFrame.deserialize(firstFrame.value);
        if (receivedFrame.isErr()) {
            return Err("invalid frame");
        }

        const frame = receivedFrame.unwrap();
        if (!frame.verifyChecksum()) {
            return Err("checksum mismatch");
        }

        if (!(frame.body instanceof SynFrameBody)) {
            return Err("invalid frame");
        }

        const socket = new TrustedSocket(tunnelSocket, localNodeService);
        const actions = socket.#state.onReceive(frame.body);
        return TrustedSocket.#open(socket, actions);
    }

    static async connect(
        tunnelSocket: TunnelSocket,
        localNodeService: LocalNodeService,
    ): Promise<Result<TrustedSocket, "timeout" | "invalid operation">> {
        const socket = new TrustedSocket(tunnelSocket, localNodeService);
        const actions = socket.#state.connect();
        if (actions.isErr()) {
            return actions;
        }

        return TrustedSocket.#open(socket, actions.unwrap());
    }

    async send(data: Uint8Array): Promise<Result<void, "timeout" | "invalid operation">> {
        const pseudoHeader = await this.#socket.createPseudoHeader();
        const result = this.#state.sendData((sequenceNumber) => {
            return TrustedFrame.create({ body: new DataFrameBody(sequenceNumber, data), pseudoHeader });
        });
        if (result.isErr()) {
            return result;
        }

        const [actions, promise] = result.unwrap();
        this.#processActions(actions);
        return promise;
    }

    async close(): Promise<Result<void, "invalid operation">> {
        const actions = this.#state.close();
        if (actions.isErr()) {
            return actions;
        }

        let cancelHandleClose: (() => void) | undefined;
        const handleClose = new Promise<void>((resolve) => {
            cancelHandleClose = this.#state.onStateChange((state) => {
                state === "closed" && resolve();
            });
        });

        this.#processActions(actions.unwrap());
        await handleClose;
        cancelHandleClose?.();
        return Ok(undefined);
    }

    async [Symbol.asyncDispose](): Promise<void> {
        await this.close();
    }

    onReceive(listener: (data: Uint8Array) => void): CancelListening {
        return this.#onReceiveData.listen(listener);
    }

    onOpen(listener: () => void): CancelListening {
        return this.#onOpen.listen(listener);
    }

    onClose(listener: () => void): CancelListening {
        return this.#onClose.listen(listener);
    }
}
