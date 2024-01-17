import { CancelListening, EventBroker } from "@core/event";
import { RETRY_COUNT, RETRY_INTERVAL } from "../constants";
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
import { Duration } from "@core/time";

export type SendResult = Result<void, "timeout">;

class SendControlAction {
    readonly type = "send";
    readonly body: SynFrameBody | FinFrameBody;
    readonly remainingRetryCount: number;
    readonly remainingAckTimeoutRetryCount: number;

    constructor(args: {
        body: SynFrameBody | FinFrameBody;
        remainingRetryCount: number;
        remainingAckTimeoutRetryCount: number;
    }) {
        this.body = args.body;
        this.remainingRetryCount = args.remainingRetryCount;
        this.remainingAckTimeoutRetryCount = args.remainingAckTimeoutRetryCount;
    }

    static create(body: SynFrameBody | FinFrameBody): SendControlAction {
        return new SendControlAction({
            body,
            remainingRetryCount: RETRY_COUNT,
            remainingAckTimeoutRetryCount: RETRY_COUNT,
        });
    }

    retryable(): boolean {
        return this.remainingRetryCount > 0;
    }

    retry(): SendControlAction {
        if (!this.retryable()) {
            throw new Error("retryable is false");
        }

        return new SendControlAction({
            body: this.body,
            remainingRetryCount: this.remainingRetryCount - 1,
            remainingAckTimeoutRetryCount: RETRY_COUNT,
        });
    }
}

class ReceiveControlAckTimeoutAction {
    readonly type = "ack timeout";
    readonly body: SynFrameBody | FinFrameBody;
    readonly remainingAckTimeoutRetryCount: number;

    constructor(args: { body: SynFrameBody | FinFrameBody; remainingAckTimeoutRetryCount: number }) {
        this.body = args.body;
        this.remainingAckTimeoutRetryCount = args.remainingAckTimeoutRetryCount;
    }

    static fromSendControlAction(action: SendControlAction): ReceiveControlAckTimeoutAction {
        return new ReceiveControlAckTimeoutAction({
            body: action.body,
            remainingAckTimeoutRetryCount: action.remainingAckTimeoutRetryCount,
        });
    }

    retryable(): boolean {
        return this.remainingAckTimeoutRetryCount > 0;
    }

    retry(): SendControlAction {
        if (!this.retryable()) {
            throw new Error("retryable is false");
        }

        return new SendControlAction({
            body: this.body,
            remainingRetryCount: RETRY_COUNT,
            remainingAckTimeoutRetryCount: this.remainingAckTimeoutRetryCount - 1,
        });
    }
}

class SendDataAction {
    readonly type = "send";
    readonly body: DataFrameBody;
    readonly remainingRetryCount: number;
    readonly remainingAckTimeoutRetryCount: number;
    readonly result: Deferred<SendResult>;

    constructor(args: {
        body: DataFrameBody;
        remainingRetryCount: number;
        remainingAckTimeoutRetryCount: number;
        result: Deferred<SendResult>;
    }) {
        this.body = args.body;
        this.remainingRetryCount = args.remainingRetryCount;
        this.remainingAckTimeoutRetryCount = args.remainingAckTimeoutRetryCount;
        this.result = args.result;
    }

    static create(body: DataFrameBody): SendDataAction {
        return new SendDataAction({
            body,
            remainingRetryCount: RETRY_COUNT,
            remainingAckTimeoutRetryCount: RETRY_COUNT,
            result: deferred(),
        });
    }

    retryable(): boolean {
        return this.remainingRetryCount > 0;
    }

    retry(): SendDataAction {
        if (!this.retryable()) {
            throw new Error("retryable is false");
        }

        return new SendDataAction({
            body: this.body,
            remainingRetryCount: this.remainingRetryCount - 1,
            remainingAckTimeoutRetryCount: RETRY_COUNT,
            result: this.result,
        });
    }
}

class ReceiveDataAckTimeoutAction {
    readonly type = "ack timeout";
    readonly body: DataFrameBody;
    readonly remainingAckTimeoutRetryCount: number;
    readonly result: Deferred<SendResult>;

    constructor(args: { body: DataFrameBody; remainingAckTimeoutRetryCount: number; result: Deferred<SendResult> }) {
        this.body = args.body;
        this.remainingAckTimeoutRetryCount = args.remainingAckTimeoutRetryCount;
        this.result = args.result;
    }

    static fromSendDataAction(action: SendDataAction): ReceiveDataAckTimeoutAction {
        return new ReceiveDataAckTimeoutAction({
            body: action.body,
            remainingAckTimeoutRetryCount: action.remainingAckTimeoutRetryCount,
            result: action.result,
        });
    }

    retryable(): boolean {
        return this.remainingAckTimeoutRetryCount > 0;
    }

    retry(): SendDataAction {
        if (!this.retryable()) {
            throw new Error("retryable is false");
        }

        return new SendDataAction({
            body: this.body,
            remainingRetryCount: RETRY_COUNT,
            remainingAckTimeoutRetryCount: this.remainingAckTimeoutRetryCount - 1,
            result: this.result,
        });
    }
}

class SendAckAction {
    readonly type = "send";
    readonly body: SynAckFrameBody | DataAckFrameBody | FinAckFrameBody;
    readonly remainingRetryCount: number;

    constructor(args: { body: SynAckFrameBody | DataAckFrameBody | FinAckFrameBody; remainingRetryCount: number }) {
        this.body = args.body;
        this.remainingRetryCount = args.remainingRetryCount;
    }

    static create(body: SynAckFrameBody | DataAckFrameBody | FinAckFrameBody): SendAckAction {
        return new SendAckAction({ body, remainingRetryCount: RETRY_COUNT });
    }

    retryable(): boolean {
        return this.remainingRetryCount < 0;
    }

    retry(): SendAckAction {
        if (!this.retryable()) {
            throw new Error("retryable is false");
        }

        return new SendAckAction({
            body: this.body,
            remainingRetryCount: this.remainingRetryCount - 1,
        });
    }
}

export type SocketAction =
    | { type: "open" }
    | { type: "close"; error: boolean }
    | { type: "receive"; data: Uint8Array }
    | SendControlAction
    | ReceiveControlAckTimeoutAction
    | SendDataAction
    | ReceiveDataAckTimeoutAction
    | SendAckAction
    | { type: "delay"; duration: Duration; actions: SocketAction[] };

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
            return Ok([SendControlAction.create(new SynFrameBody())]);
        } else {
            return Err("invalid operation");
        }
    }

    #isOpen(): boolean {
        return this.#incoming === "syn ack sent" && this.#outgoing === "syn ack received";
    }

    onSynSendSuccess(action: SendControlAction): SocketAction[] {
        this.#outgoing = "syn sent";
        return [ReceiveControlAckTimeoutAction.fromSendControlAction(action)];
    }

    onSynSendFailure(action: SendControlAction): SocketAction[] {
        if (this.#outgoing !== "yet") {
            return [];
        }

        if (!action.retryable()) {
            return [{ type: "close", error: true }];
        }

        return [{ type: "delay", duration: RETRY_INTERVAL, actions: [action.retry()] }];
    }

    onSynAckReceived(): SocketAction[] {
        if (this.#outgoing === "yet") {
            return [];
        }

        this.#outgoing = "syn ack received";
        return this.#isOpen() ? [{ type: "open" }] : [];
    }

    onSynAckReceiveTimeout(action: ReceiveControlAckTimeoutAction): SocketAction[] {
        if (this.#outgoing !== "syn sent") {
            return [];
        }

        if (!action.retryable()) {
            return [{ type: "close", error: true }];
        }

        return [action.retry()];
    }

    onSynReceived(): SocketAction[] {
        this.#incoming = "syn received";
        if (this.#outgoing === "yet") {
            return [SendAckAction.create(new SynAckFrameBody()), SendControlAction.create(new SynFrameBody())];
        } else {
            return [SendAckAction.create(new SynAckFrameBody())];
        }
    }

    onSynAckSendSuccess(): SocketAction[] {
        if (this.#incoming !== "syn received") {
            return [];
        }

        this.#incoming = "syn ack sent";
        return this.#isOpen() ? [{ type: "open" }] : [];
    }

    onSynAckSendFailure(action: SendAckAction): SocketAction[] {
        if (this.#incoming !== "syn received") {
            return [];
        }

        if (!action.retryable()) {
            return [{ type: "close", error: true }];
        }

        return [{ type: "delay", duration: RETRY_INTERVAL, actions: [action.retry()] }];
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

        if (body.sequenceNumber.isOlderThan(this.#nextReceiveSequenceNumber)) {
            return [SendAckAction.create(new DataAckFrameBody(body.sequenceNumber))];
        }

        if (!body.sequenceNumber.equals(this.#nextReceiveSequenceNumber)) {
            return [];
        }

        this.#nextReceiveSequenceNumber = this.#nextReceiveSequenceNumber.next();
        return [
            { type: "receive", data: body.payload },
            SendAckAction.create(new DataAckFrameBody(body.sequenceNumber)),
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

        return [SendDataAction.create(this.#sendingFrame.frame.body)];
    }

    onReceiveDataAckTimeout(action: ReceiveDataAckTimeoutAction): SocketAction[] {
        if (!this.#isOpen || this.#sendingFrame === undefined) {
            return [];
        }

        if (!action.body.sequenceNumber.equals(this.#sendingFrame.frame.body.sequenceNumber)) {
            return [];
        }

        if (!action.retryable()) {
            this.#sendingFrame.result.resolve(Err("timeout"));
            return [{ type: "close", error: true }];
        }

        return [{ type: "delay", duration: RETRY_INTERVAL, actions: [action.retry()] }];
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
        return Ok([[SendDataAction.create(frame.body)], result]);
    }

    onSendDataSuccess(action: SendDataAction): SocketAction[] {
        if (!this.#isOpen || this.#sendingFrame === undefined) {
            return [];
        }

        return [ReceiveDataAckTimeoutAction.fromSendDataAction(action)];
    }

    onSendDataFailure(action: SendDataAction): SocketAction[] {
        if (!this.#isOpen || this.#sendingFrame === undefined) {
            return [];
        }

        if (!action.body.sequenceNumber.equals(this.#sendingFrame.frame.body.sequenceNumber)) {
            return [];
        }

        if (!action.retryable()) {
            action.result.resolve(Err("timeout"));
            return [{ type: "close", error: true }];
        }

        return [{ type: "delay", duration: RETRY_INTERVAL, actions: [action.retry()] }];
    }

    onSendDataAckSuccess(): SocketAction[] {
        return [];
    }

    onSendDataAckFailure(action: SendAckAction): SocketAction[] {
        if (!this.#isOpen || this.#sendingFrame === undefined) {
            return [];
        }

        if (!action.retryable()) {
            return [{ type: "close", error: true }];
        }

        return [{ type: "delay", duration: RETRY_INTERVAL, actions: [action.retry()] }];
    }

    onOpen(): void {
        this.#isOpen = true;
    }

    onClose(): void {
        this.#isOpen = false;
        this.#nextReceiveSequenceNumber = SequenceNumber.zero();
        this.#nextSendSequenceNumber = SequenceNumber.zero();
        this.#sendingFrame = undefined;

        for (const frame of this.#sendPendingFrames) {
            frame.result.resolve(Err("timeout"));
        }
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

    onFinSendSuccess(action: SendControlAction): SocketAction[] {
        this.#outgoing = "fin sent";
        return [ReceiveControlAckTimeoutAction.fromSendControlAction(action)];
    }

    onFinSendFailure(action: SendControlAction): SocketAction[] {
        if (this.#outgoing !== "yet") {
            return [];
        }

        if (!action.retryable()) {
            return [{ type: "close", error: true }];
        }

        return [{ type: "delay", duration: RETRY_INTERVAL, actions: [action.retry()] }];
    }

    onFinAckReceived(): SocketAction[] {
        if (this.#outgoing === "yet") {
            return [];
        }

        this.#outgoing = "fin ack received";
        return this.#isClosed() ? [{ type: "close", error: false }] : [];
    }

    onFinAckReceiveTimeout(action: ReceiveControlAckTimeoutAction): SocketAction[] {
        if (this.#outgoing !== "fin sent") {
            return [];
        }

        if (!action.retryable()) {
            return [{ type: "close", error: true }];
        }

        return [action.retry()];
    }

    onFinReceived(): SocketAction[] {
        this.#incoming = "fin received";
        if (this.#outgoing === "yet") {
            return [SendAckAction.create(new FinAckFrameBody()), SendControlAction.create(new FinFrameBody())];
        } else {
            return [SendAckAction.create(new FinAckFrameBody())];
        }
    }

    onFinAckSendSuccess(): SocketAction[] {
        if (this.#incoming !== "fin received") {
            return [];
        }

        this.#incoming = "fin ack sent";
        return this.#isClosed() ? [{ type: "close", error: false }] : [];
    }

    onFinAckSendFailure(action: SendAckAction): SocketAction[] {
        if (this.#incoming !== "fin received") {
            return [];
        }

        if (action.remainingRetryCount === 0) {
            return [{ type: "close", error: true }];
        }

        return [{ type: "delay", duration: RETRY_INTERVAL, actions: [action.retry()] }];
    }

    close(): OperationResult {
        if (this.#outgoing === "yet" && this.#incoming === "yet") {
            return Ok([SendControlAction.create(new FinFrameBody())]);
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

    onSendSuccess(action: SendControlAction | SendDataAction | SendAckAction): SocketAction[] {
        return this.#socketActionHook(
            match(action)
                .with({ body: P.instanceOf(SynFrameBody) }, (action) => this.#syn.onSynSendSuccess(action))
                .with({ body: P.instanceOf(SynAckFrameBody) }, () => this.#syn.onSynAckSendSuccess())
                .with({ body: P.instanceOf(DataFrameBody) }, (action) => this.#data.onSendDataSuccess(action))
                .with({ body: P.instanceOf(DataAckFrameBody) }, () => this.#data.onSendDataAckSuccess())
                .with({ body: P.instanceOf(FinFrameBody) }, (action) => this.#fin.onFinSendSuccess(action))
                .with({ body: P.instanceOf(FinAckFrameBody) }, () => this.#fin.onFinAckSendSuccess())
                .exhaustive(),
        );
    }

    onSendFailure(action: SendControlAction | SendDataAction | SendAckAction): SocketAction[] {
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

    onReceiveAckTimeout(action: ReceiveControlAckTimeoutAction | ReceiveDataAckTimeoutAction): SocketAction[] {
        return this.#socketActionHook(
            match(action)
                .with({ body: P.instanceOf(SynFrameBody) }, (action) => this.#syn.onSynAckReceiveTimeout(action))
                .with({ body: P.instanceOf(DataFrameBody) }, (action) => this.#data.onReceiveDataAckTimeout(action))
                .with({ body: P.instanceOf(FinFrameBody) }, (action) => this.#fin.onFinAckReceiveTimeout(action))
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
