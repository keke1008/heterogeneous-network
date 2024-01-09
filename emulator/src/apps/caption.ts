import { CaptionServer, CreateBlob } from "@core/apps/caption";
import { TrustedService } from "@core/net";

const createBlob: CreateBlob = (data, { fontSize }) => {
    const canvas = document.createElement("canvas");
    const ctx = canvas.getContext("2d");
    if (ctx === null) {
        throw new Error("Failed to create canvas context");
    }

    ctx.font = `${fontSize}px sans-serif`;
    const textMetrics = ctx.measureText(data);
    const width = textMetrics.width;
    const height = textMetrics.actualBoundingBoxAscent + textMetrics.actualBoundingBoxDescent;

    canvas.width = width;
    canvas.height = height;

    ctx.fillText(data, 0, height - textMetrics.actualBoundingBoxDescent);

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
