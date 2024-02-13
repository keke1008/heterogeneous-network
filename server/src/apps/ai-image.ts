import {
    AI_IMAGE_HTTP_PATH,
    AI_IMAGE_HTTP_METHOD,
    aiImageHttpRequestBody,
    AiImageHttpRequestBody,
    AiImageHttpResponseBody,
    AI_IMAGE_HTTP_PORT,
} from "@core/apps/ai-image";
import OpenAI from "openai";
import { createServer, Server } from "@core/httpServer";

const openai = new OpenAI();

export interface GeneratedImage {
    uri: string;
    prompt: string;
}

export class AiImageGenerationServer {
    #server: Server;

    constructor() {
        this.#server = createServer(async (req, res) => {
            res.setHeader("Access-Control-Allow-Origin", "*");

            if (req.method !== AI_IMAGE_HTTP_METHOD || req.url !== AI_IMAGE_HTTP_PATH) {
                console.warn(`Invalid request method or path: ${req.method} ${req.url}`);
                res.writeHead(404);
                res.end();
                return;
            }

            console.info("Received image generation request", req.url);
            const body = await new Promise<AiImageHttpRequestBody>((resolve, reject) => {
                req.setEncoding("utf8");
                req.on("data", (chunk: string) => {
                    resolve(aiImageHttpRequestBody.parse(JSON.parse(chunk)));
                });
                req.on("error", (err) => reject(err));
                req.on("close", () => reject("Connection closed"));
            }).catch((error) => ({ error }));
            if ("error" in body) {
                console.warn("Failed to parse request body", body.error);
                res.writeHead(400);
                res.end();
                return;
            }

            console.info("Generating image for prompt:", body.prompt);
            const imageResponse = await openai.images
                .generate({
                    model: "dall-e-2",
                    prompt: body.prompt,
                    n: 1,
                    size: "1024x1024",
                    response_format: "url",
                })
                .catch((error) => ({ error }));
            if ("error" in imageResponse) {
                console.error("Failed to generate image", imageResponse.error);
                res.writeHead(500);
                res.end();
                return;
            }

            console.info("Generated image:", imageResponse.data[0].url);
            res.writeHead(200, { "Content-Type": "application/json" });
            const response: AiImageHttpResponseBody = { imageUrl: imageResponse.data[0].url! };
            res.end(JSON.stringify(response));
        });

        this.#server.on("listening", () => {
            console.info(`AI image generation server is listening on port ${AI_IMAGE_HTTP_PORT}`);
        });
        this.#server.listen(AI_IMAGE_HTTP_PORT);
    }

    close() {
        this.#server.close();
    }
}
