import { BufferReader, BufferWriter, Destination, TrustedService, TrustedSocket, TunnelPortId } from "@core/net";
import {
    EnumSerdeable,
    ObjectSerdeable,
    SerdeableValue,
    TransformSerdeable,
    Uint16Serdeable,
    Uint32Serdeable,
    VariantSerdeable,
} from "@core/serde";
import { Utf8Serdeable } from "@core/serde/utf8";
import { Ok, Result } from "oxide.ts";
import { P, match } from "ts-pattern";

export const CaptionRenderOptions = {
    serdeable: new ObjectSerdeable({
        x: new Uint32Serdeable(),
        y: new Uint32Serdeable(),
        font: Utf8Serdeable,
        fontSize: new Uint32Serdeable(),
        color: Utf8Serdeable,
        alignment: new TransformSerdeable(
            Utf8Serdeable,
            (alignment) => {
                if (alignment === "left" || alignment === "center" || alignment === "right") {
                    return alignment;
                }
            },
            (alignment) => alignment,
        ),
        lineSpacing: new Uint32Serdeable(),
        text: Utf8Serdeable,
    }),
};
export type CaptionRenderOptions = SerdeableValue<typeof CaptionRenderOptions.serdeable>;
export type PositionOmittedCaptionRenderOptions = Omit<CaptionRenderOptions, "x" | "y">;

const CAPTION_PORT = TunnelPortId.schema.parse(11);

export type CreateBlob = (options: PositionOmittedCaptionRenderOptions) => Promise<Blob>;

export class ServerInfo {
    address: string;
    port: number;

    constructor(args: { address: string; port: number }) {
        this.address = args.address;
        this.port = args.port;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ address: Utf8Serdeable, port: new Uint16Serdeable() }),
        (params) => new ServerInfo(params),
        (params) => params,
    );

    toOrigin(): string {
        return `http://${this.address}:${this.port}`;
    }
}

enum ParamsFrameType {
    ShowCaption = 1,
    ClearCaption = 2,
}

export class ShowCaption {
    readonly type = ParamsFrameType.ShowCaption;
    server: ServerInfo;
    options: CaptionRenderOptions;

    constructor(args: { server: ServerInfo; options: CaptionRenderOptions }) {
        this.server = args.server;
        this.options = args.options;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            server: ServerInfo.serdeable,
            options: CaptionRenderOptions.serdeable,
        }),
        (params) => new ShowCaption(params),
        (params) => params,
    );

    async createFormData(createBlob: CreateBlob): Promise<FormData> {
        const formData = new FormData();
        formData.append("x", this.options.x.toString());
        formData.append("y", this.options.y.toString());
        formData.append("image", await createBlob(this.options));
        return formData;
    }

    toHttpMethod(): string {
        return "POST";
    }
}

export class ClearCaption {
    readonly type = ParamsFrameType.ClearCaption;
    server: ServerInfo;

    constructor(args: { server: ServerInfo }) {
        this.server = args.server;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({ server: ServerInfo.serdeable }),
        (params) => new ClearCaption(params),
        (params) => params,
    );

    toHttpMethod(): string {
        return "DELETE";
    }
}

export const ParamsFrameSerdeable = new VariantSerdeable(
    [ShowCaption.serdeable, ClearCaption.serdeable] as const,
    (frame) => frame.type,
);

export enum CaptionStatus {
    Success,
    Failure,
}

export class CaptionResult {
    status: CaptionStatus;

    constructor(args: { status: CaptionStatus }) {
        this.status = args.status;
    }

    static readonly serdeable = new TransformSerdeable(
        new EnumSerdeable(CaptionStatus),
        (status) => new CaptionResult({ status }),
        (params) => params.status,
    );
}

export class CaptionServer {
    #service: TrustedService;
    #createBlob: CreateBlob;
    #cancel?: () => void;

    constructor(service: TrustedService, createBlob: CreateBlob) {
        this.#service = service;
        this.#createBlob = createBlob;
    }

    sendResponse(socket: TrustedSocket, status: CaptionStatus): Promise<Result<void, "timeout" | "invalid operation">> {
        const res = new CaptionResult({ status });
        const buf = BufferWriter.serialize(CaptionResult.serdeable.serializer(res)).unwrap();
        return socket.send(buf);
    }

    start(): Result<void, "already opened"> {
        const result = this.#service.listen(CAPTION_PORT, (socket) => {
            socket.onReceive(async (data) => {
                const deserializeResult = BufferReader.deserialize(ParamsFrameSerdeable.deserializer(), data);
                console.info("Received params", deserializeResult);
                if (deserializeResult.isErr()) {
                    console.info("Failed to deserialize params", deserializeResult.unwrapErr());
                    return;
                }

                const params = deserializeResult.unwrap();
                let body;
                try {
                    body = await match(params)
                        .with(P.instanceOf(ShowCaption), (params) => params.createFormData(this.#createBlob))
                        .with(P.instanceOf(ClearCaption), () => undefined)
                        .exhaustive();
                } catch (e) {
                    console.info("Failed to create form data", e);
                    await this.sendResponse(socket, CaptionStatus.Failure);
                    return;
                }

                const response = await Result.safe(
                    fetch(params.server.toOrigin(), { method: params.toHttpMethod(), body }),
                );
                const success = response.isOk() && response.unwrap().ok;
                if (!success) {
                    console.info("Failed to fetch caption", response);
                }

                this.sendResponse(socket, success ? CaptionStatus.Success : CaptionStatus.Failure);
            });
        });

        if (result.isErr()) {
            return result;
        }
        this.#cancel = result.unwrap();
        return Ok(undefined);
    }

    close(): void {
        this.#cancel?.();
        this.#cancel = undefined;
    }

    [Symbol.dispose](): void {
        this.close();
    }
}

export class CaptionClient {
    #socket: TrustedSocket;

    private constructor(args: { socket: TrustedSocket }) {
        this.#socket = args.socket;
    }

    static async connect(args: { trustedService: TrustedService; destination: Destination }) {
        const socket = await args.trustedService.connect({
            destination: args.destination,
            destinationPortId: CAPTION_PORT,
        });
        return socket.map((socket) => new CaptionClient({ socket }));
    }

    async send(params: ShowCaption | ClearCaption): Promise<CaptionResult> {
        const buf = BufferWriter.serialize(ParamsFrameSerdeable.serializer(params)).unwrap();
        await this.#socket.send(buf);

        return new Promise<CaptionResult>((resolve) => {
            const cancel = this.#socket.onReceive((data) => {
                cancel();
                resolve(BufferReader.deserialize(CaptionResult.serdeable.deserializer(), data).unwrap());
            });
        });
    }

    async close() {
        await this.#socket.close();
    }

    onClose(callback: () => void) {
        return this.#socket.onClose(callback);
    }
}
