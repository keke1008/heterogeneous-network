import { CancelListening, EventBroker } from "@core/event";
import { RETRY_COUNT } from "../constants";
import {
    DataAckFrameBody,
    DataFrameBody,
    FinAckFrameBody,
    FinFrameBody,
    SynAckFrameBody,
    SynFrameBody,
    TrustedDataFrame,
    TrustedFrameBody,
} from "../frame";
import { SequenceNumber } from "../sequenceNumber";
import { P, match } from "ts-pattern";
import { Err, Ok, Result } from "oxide.ts";
import { Deferred, deferred } from "@core/deferred";

export type SendResult = Result<void, "timeout">;
export type SendAction = { body: Exclude<TrustedFrameBody, DataFrameBody>; remainingRetryCount: number };
export type SendDataAction = {
    body: DataFrameBody;
    remainingRetryCount: number;
    result: Deferred<SendResult>;
};

export type SocketAction =
    | { type: "open" }
    | { type: "close"; error: boolean }
    | { type: "receive"; data: Uint8Array }
    | ({ type: "send" } & SendAction)
    | ({ type: "send after interval" } & SendAction)
    | ({ type: "send data" } & SendDataAction)
    | ({ type: "send data after interval" } & SendDataAction);

export type OperationResult = Result<SocketAction[], "invalid operation">;

type SynIncomingState = "yet" | "syn received" | "syn ack sent";
type SynOutgoingState = "yet" | "syn sent" | "syn ack received";
class SynSocketState {
    #incoming: SynIncomingState = "yet";
    #outgoing: SynOutgoingState = "yet";

    isOpening(): boolean {
        return (
            !(this.#incoming === "syn ack sent" && this.#outgoing === "syn ack received") &&
            !(this.#incoming === "yet" || this.#outgoing === "yet")
        );
    }

    connect(): OperationResult {
        if (this.#incoming === "yet" && this.#outgoing === "yet") {
            return Ok([{ type: "send", body: new SynFrameBody(), remainingRetryCount: RETRY_COUNT }]);
        } else {
            return Err("invalid operation");
        }
    }

    #isOpen(): boolean {
        return this.#incoming === "syn ack sent" && this.#outgoing === "syn ack received";
    }

    onSynSendSuccess(): SocketAction[] {
        this.#outgoing = "syn sent";
        return [];
    }

    onSynSendFailure(action: SendAction): SocketAction[] {
        if (this.#outgoing !== "yet") {
            return [];
        }

        if (action.remainingRetryCount === 0) {
            return [{ type: "close", error: true }];
        }

        const remainingRetryCount = action.remainingRetryCount - 1;
        return [{ type: "send after interval", body: action.body, remainingRetryCount }];
    }

    onSynAckReceived(): SocketAction[] {
        if (this.#outgoing === "yet") {
            return [];
        }

        this.#outgoing = "syn ack received";
        return this.#isOpen() ? [{ type: "open" }] : [];
    }

    onSynReceived(): SocketAction[] {
        this.#incoming = "syn received";
        const synAckAction = { type: "send", body: new SynAckFrameBody(), remainingRetryCount: RETRY_COUNT } as const;
        if (this.#outgoing === "yet") {
            const synAction = { type: "send", body: new SynFrameBody(), remainingRetryCount: RETRY_COUNT } as const;
            return [synAckAction, synAction];
        } else {
            return [synAckAction];
        }
    }

    onSynAckSendSuccess(): SocketAction[] {
        if (this.#incoming !== "syn received") {
            return [];
        }

        this.#incoming = "syn ack sent";
        return this.#isOpen() ? [{ type: "open" }] : [];
    }

    onSynAckSendFailure(action: SendAction): SocketAction[] {
        if (this.#incoming !== "syn received") {
            return [];
        }

        if (action.remainingRetryCount === 0) {
            return [{ type: "close", error: true }];
        }

        const remainingRetryCount = action.remainingRetryCount - 1;
        return [{ type: "send after interval", body: action.body, remainingRetryCount }];
    }

    onClose(): void {
        this.#incoming = "yet";
        this.#outgoing = "yet";
    }
}

interface DataFrame {
    frame: TrustedDataFrame;
    result: Deferred<Result<void, "timeout">>;
}
class DataSocketState {
    #isOpen = false;

    #nextReceiveSequenceNumber = SequenceNumber.zero();

    #nextSendSequenceNumber = SequenceNumber.zero();
    #sendingFrame?: DataFrame;
    #sendPendingFrames: DataFrame[] = [];

    onDataReceived(body: DataFrameBody): SocketAction[] {
        if (!this.#isOpen) {
            return [];
        }

        if (!body.sequenceNumber.equals(this.#nextReceiveSequenceNumber)) {
            return [];
        }

        this.#nextReceiveSequenceNumber = this.#nextReceiveSequenceNumber.next();
        return [
            { type: "receive", data: body.payload },
            { type: "send", body: new DataAckFrameBody(body.sequenceNumber), remainingRetryCount: RETRY_COUNT },
        ];
    }

    onDataAckReceived(body: DataAckFrameBody): SocketAction[] {
        if (!this.#isOpen) {
            return [];
        }

        if (this.#sendingFrame === undefined) {
            return [];
        }

        if (!body.sequenceNumber.equals(this.#sendingFrame.frame.body.sequenceNumber)) {
            return [];
        }

        this.#sendingFrame.result.resolve(Ok(undefined));
        this.#sendingFrame = this.#sendPendingFrames.shift();
        if (this.#sendingFrame === undefined) {
            return [];
        }

        return [
            {
                type: "send data",
                body: this.#sendingFrame.frame.body,
                remainingRetryCount: RETRY_COUNT,
                result: this.#sendingFrame.result,
            },
        ];
    }

    sendData(
        createFrame: (sequenceNumber: SequenceNumber) => TrustedDataFrame,
    ): Result<[SocketAction[], Deferred<SendResult>], "invalid operation"> {
        if (!this.#isOpen) {
            return Err("invalid operation");
        }

        const frame = createFrame(this.#nextSendSequenceNumber);
        this.#nextSendSequenceNumber = this.#nextSendSequenceNumber.next();
        const result = deferred<SendResult>();

        if (this.#sendingFrame !== undefined) {
            this.#sendPendingFrames.push({ frame, result });
            return Ok([[], result]);
        }

        this.#sendingFrame = { frame, result };
        const action = { type: "send data", body: frame.body, remainingRetryCount: RETRY_COUNT, result } as const;
        return Ok([[action], result]);
    }

    onSendDataSuccess(): SocketAction[] {
        return [];
    }

    onSendDataFailure(action: SendDataAction): SocketAction[] {
        if (!this.#isOpen) {
            return [];
        }

        if (this.#sendingFrame === undefined) {
            return [];
        }

        if (!action.body.sequenceNumber.equals(this.#sendingFrame.frame.body.sequenceNumber)) {
            return [];
        }

        if (action.remainingRetryCount === 0) {
            action.result.resolve(Err("timeout"));
            return [{ type: "close", error: true }];
        }

        const remainingRetryCount = action.remainingRetryCount - 1;
        return [{ type: "send data after interval", body: action.body, remainingRetryCount, result: action.result }];
    }

    onSendDataAckSuccess(): SocketAction[] {
        return [];
    }

    onSendDataAckFailure(action: SendAction): SocketAction[] {
        if (!this.#isOpen) {
            return [];
        }

        if (this.#sendingFrame === undefined) {
            return [];
        }

        if (action.remainingRetryCount === 0) {
            return [{ type: "close", error: true }];
        }

        const remainingRetryCount = action.remainingRetryCount - 1;
        return [{ type: "send after interval", body: action.body, remainingRetryCount }];
    }

    onOpen(): void {
        this.#isOpen = true;
    }

    onClose(): void {
        this.#isOpen = false;
        this.#nextReceiveSequenceNumber = SequenceNumber.zero();
        this.#nextSendSequenceNumber = SequenceNumber.zero();
        this.#sendingFrame = undefined;
        this.#sendPendingFrames = [];
    }
}

type FinIncomingState = "yet" | "fin received" | "fin ack sent";
type FinOutgoingState = "yet" | "fin sent" | "fin ack received";
class FinSocketState {
    #incoming: FinIncomingState = "yet";
    #outgoing: FinOutgoingState = "yet";

    isClosing(): boolean {
        return (
            !(this.#incoming === "fin ack sent" && this.#outgoing === "fin ack received") &&
            !(this.#incoming === "yet" && this.#outgoing === "yet")
        );
    }

    #isClosed(): boolean {
        return this.#incoming === "fin ack sent" && this.#outgoing === "fin ack received";
    }

    onFinReceived(): SocketAction[] {
        this.#incoming = "fin received";
        const finAckAction = { type: "send", body: new FinAckFrameBody(), remainingRetryCount: RETRY_COUNT } as const;
        if (this.#outgoing === "yet") {
            const finAction = { type: "send", body: new FinFrameBody(), remainingRetryCount: RETRY_COUNT } as const;
            return [finAckAction, finAction];
        } else {
            return [finAckAction];
        }
    }

    onFinAckSendSuccess(): SocketAction[] {
        if (this.#incoming !== "fin received") {
            return [];
        }

        this.#incoming = "fin ack sent";
        return this.#isClosed() ? [{ type: "close", error: false }] : [];
    }

    onFinAckSendFailure(action: SendAction): SocketAction[] {
        if (this.#incoming !== "fin received") {
            return [];
        }

        if (action.remainingRetryCount === 0) {
            return [{ type: "close", error: true }];
        }

        const remainingRetryCount = action.remainingRetryCount - 1;
        return [{ type: "send after interval", body: action.body, remainingRetryCount }];
    }

    onFinSendSuccess(): SocketAction[] {
        this.#outgoing = "fin sent";
        return [];
    }

    onFinSendFailure(action: SendAction): SocketAction[] {
        if (this.#outgoing !== "yet") {
            return [];
        }

        if (action.remainingRetryCount === 0) {
            return [{ type: "close", error: true }];
        }

        const remainingRetryCount = action.remainingRetryCount - 1;
        return [{ type: "send after interval", body: action.body, remainingRetryCount }];
    }

    onFinAckReceived(): SocketAction[] {
        if (this.#outgoing === "yet") {
            return [];
        }

        this.#outgoing = "fin ack received";
        return this.#isClosed() ? [{ type: "close", error: false }] : [];
    }

    close(): OperationResult {
        if (this.#outgoing === "yet" && this.#incoming === "yet") {
            return Ok([
                { type: "send", body: new FinFrameBody(), remainingRetryCount: RETRY_COUNT, result: deferred() },
            ]);
        } else {
            return Err("invalid operation");
        }
    }

    onClose(): void {
        this.#incoming = "yet";
        this.#outgoing = "yet";
    }
}

export type StateType = "opening" | "open" | "closing" | "closed";

export class SocketState {
    #state: StateType = "closed";
    #onStateChange = new EventBroker<StateType>();
    #onError = new EventBroker<void>();

    #syn = new SynSocketState();
    #data = new DataSocketState();
    #fin = new FinSocketState();

    #socketActionHook(actions: SocketAction[]): SocketAction[] {
        const prev = this.#state;

        for (const action of actions) {
            match(action)
                .with({ type: "open" }, () => {
                    this.#state = "open";
                    this.#data.onOpen();
                })
                .with({ type: "close" }, ({ error }) => {
                    this.#state = "closed";
                    this.#syn.onClose();
                    this.#data.onClose();
                    this.#fin.onClose();

                    error && this.#onError.emit();
                })
                .otherwise(() => {});
        }

        if (this.#syn.isOpening()) {
            this.#state = "opening";
        }

        if (this.#fin.isClosing()) {
            this.#state = "closing";
        }

        if (prev !== this.#state) {
            this.#onStateChange.emit(this.#state);
        }

        return actions;
    }

    onStateChange(callback: (state: StateType) => void): CancelListening {
        return this.#onStateChange.listen(callback);
    }

    onError(callback: () => void): CancelListening {
        return this.#onError.listen(callback);
    }

    onReceive(body: TrustedFrameBody): SocketAction[] {
        return this.#socketActionHook(
            match(body)
                .with(P.instanceOf(SynFrameBody), () => this.#syn.onSynReceived())
                .with(P.instanceOf(SynAckFrameBody), () => this.#syn.onSynAckReceived())
                .with(P.instanceOf(DataFrameBody), (body) => this.#data.onDataReceived(body))
                .with(P.instanceOf(DataAckFrameBody), (body) => this.#data.onDataAckReceived(body))
                .with(P.instanceOf(FinFrameBody), () => this.#fin.onFinReceived())
                .with(P.instanceOf(FinAckFrameBody), () => this.#fin.onFinAckReceived())
                .exhaustive(),
        );
    }

    onSendSuccess(action: SendAction | SendDataAction): SocketAction[] {
        return this.#socketActionHook(
            match(action.body)
                .with(P.instanceOf(SynFrameBody), () => this.#syn.onSynSendSuccess())
                .with(P.instanceOf(SynAckFrameBody), () => this.#syn.onSynAckSendSuccess())
                .with(P.instanceOf(DataFrameBody), () => this.#data.onSendDataSuccess())
                .with(P.instanceOf(DataAckFrameBody), () => this.#data.onSendDataAckSuccess())
                .with(P.instanceOf(FinFrameBody), () => this.#fin.onFinSendSuccess())
                .with(P.instanceOf(FinAckFrameBody), () => this.#fin.onFinAckSendSuccess())
                .exhaustive(),
        );
    }

    onSendFailure(action: SendAction | SendDataAction): SocketAction[] {
        return this.#socketActionHook(
            match(action)
                .with({ body: P.instanceOf(SynFrameBody) }, (action) => this.#syn.onSynSendFailure(action))
                .with({ body: P.instanceOf(SynAckFrameBody) }, (action) => this.#syn.onSynAckSendFailure(action))
                .with({ body: P.instanceOf(DataFrameBody) }, (action) => this.#data.onSendDataFailure(action))
                .with({ body: P.instanceOf(DataAckFrameBody) }, (action) => this.#data.onSendDataAckFailure(action))
                .with({ body: P.instanceOf(FinFrameBody) }, (action) => this.#fin.onFinSendFailure(action))
                .with({ body: P.instanceOf(FinAckFrameBody) }, (action) => this.#fin.onFinAckSendFailure(action))
                .exhaustive(),
        );
    }

    connect(): OperationResult {
        return this.#syn.connect().map((actions) => this.#socketActionHook(actions));
    }

    sendData(createFrame: (sequenceNumber: SequenceNumber) => TrustedDataFrame) {
        return this.#data
            .sendData(createFrame)
            .map(([actions, result]) => [this.#socketActionHook(actions), result] as const);
    }

    close(): OperationResult {
        return this.#fin.close().map((actions) => this.#socketActionHook(actions));
    }
}
