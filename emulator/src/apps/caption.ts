import { CaptionServer, CreateBlob, PositionOmittedCaptionRenderOptions } from "@core/apps/caption";
import { TrustedService } from "@core/net";

export const getRenderedCaptionPropeties = (options: PositionOmittedCaptionRenderOptions) => {
    const canvas = document.createElement("canvas");
    const ctx = canvas.getContext("2d");
    if (ctx === null) {
        throw new Error("Failed to create canvas context");
    }

    ctx.font = `${options.fontSize}px ${options.font}`;
    const textMetrics = ctx.measureText(options.text);
    const width = textMetrics.width;
    const height = textMetrics.actualBoundingBoxAscent + textMetrics.actualBoundingBoxDescent;

    return { width, height, x: 0, y: height - textMetrics.actualBoundingBoxDescent };
};

export const renderCaption = (canvas: HTMLCanvasElement, options: PositionOmittedCaptionRenderOptions) => {
    const ctx = canvas.getContext("2d");
    if (ctx === null) {
        throw new Error("Failed to create canvas context");
    }

    const { x, y, width, height } = getRenderedCaptionPropeties(options);
    canvas.width = width;
    canvas.height = height;

    ctx.font = `${options.fontSize}px ${options.font}`;
    ctx.fillStyle = options.color;
    ctx.fillText(options.text, x, y);
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
