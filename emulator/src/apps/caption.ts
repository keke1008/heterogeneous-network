import { CaptionServer, CreateBlob, PositionOmittedCaptionRenderOptions } from "@core/apps/caption";
import { TrustedService } from "@core/net";
import { match } from "ts-pattern";

const setContextProperties = (ctx: CanvasRenderingContext2D, options: PositionOmittedCaptionRenderOptions) => {
    ctx.font = `${options.fontSize}px ${options.font}`;
    ctx.textAlign = options.alignment;
    ctx.fillStyle = options.color;
};

const getCanvasContext = (canvas: HTMLCanvasElement, options: PositionOmittedCaptionRenderOptions) => {
    const ctx = canvas.getContext("2d");
    if (ctx === null) {
        throw new Error("Failed to create canvas context");
    }

    setContextProperties(ctx, options);
    return ctx;
};

const getSingleLineCaptionSize = (ctx: CanvasRenderingContext2D, text: string) => {
    const textMetrics = ctx.measureText(text);
    return {
        width: textMetrics.actualBoundingBoxLeft + textMetrics.actualBoundingBoxRight,
        height: textMetrics.fontBoundingBoxAscent + textMetrics.fontBoundingBoxDescent,
    };
};

const getSingleLineCaptionProperties = (ctx: CanvasRenderingContext2D, maxWidth: number, text: string) => {
    const textMetrics = ctx.measureText(text);
    const x = match(ctx.textAlign)
        .with("left", () => textMetrics.actualBoundingBoxLeft)
        .with("center", () => maxWidth / 2)
        .with("right", () => maxWidth - textMetrics.actualBoundingBoxRight)
        .run();

    const { width, height } = getSingleLineCaptionSize(ctx, text);
    return {
        x,
        y: textMetrics.fontBoundingBoxAscent,
        width,
        height,
        marginTop: textMetrics.fontBoundingBoxAscent - textMetrics.actualBoundingBoxAscent,
        marginBottom: textMetrics.fontBoundingBoxDescent - textMetrics.actualBoundingBoxDescent,
    };
};

const calculateCaptionRenderingPropeties = (
    ctx: CanvasRenderingContext2D,
    options: PositionOmittedCaptionRenderOptions,
) => {
    const rows = options.text.split("\n");
    const maxWidth = rows
        .map((text) => getSingleLineCaptionSize(ctx, text))
        .reduce((maxWidth, { width }) => Math.max(maxWidth, width), 0);

    const texts = [];
    let accumulateY = 0;
    for (const text of rows) {
        const { x, y, height, marginTop, marginBottom } = getSingleLineCaptionProperties(ctx, maxWidth, text);
        texts.push({ text, x, y: accumulateY + y, marginTop, marginBottom });
        accumulateY += height + options.lineSpacing;
    }

    const marginTop = texts.at(0)?.marginTop ?? 0;
    for (const text of texts) {
        text.y -= marginTop;
    }

    const margin = (texts.at(0)?.marginTop ?? 0) + (texts.at(-1)?.marginBottom ?? 0);
    const space = margin + options.lineSpacing;
    return { width: maxWidth, height: Math.max(1, accumulateY - space), texts };
};

export const getCaptionRenderingPropeties = (options: PositionOmittedCaptionRenderOptions) => {
    const canvas = document.createElement("canvas");
    const ctx = getCanvasContext(canvas, options);
    return calculateCaptionRenderingPropeties(ctx, options);
};

export const renderCaption = (canvas: HTMLCanvasElement, options: PositionOmittedCaptionRenderOptions) => {
    const ctx = getCanvasContext(canvas, options);

    const { width, height, texts } = calculateCaptionRenderingPropeties(ctx, options);
    canvas.width = width;
    canvas.height = height;

    setContextProperties(ctx, options);
    for (const { text, x, y } of texts) {
        ctx.fillText(text, x, y);
    }
};

const createBlob: CreateBlob = (options) => {
    const canvas = document.createElement("canvas");
    renderCaption(canvas, options);

    return new Promise((resolve, reject) => {
        canvas.toBlob((blob) => {
            blob === null ? reject(new Error("Failed to create blob")) : resolve(blob);
        });
    });
};

export const createCaptionServer = (service: TrustedService): CaptionServer => {
    const server = new CaptionServer(service, createBlob);
    server.start();
    return server;
};
