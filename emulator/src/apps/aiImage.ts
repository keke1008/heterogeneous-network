import {
    AI_IMAGE_HTTP_METHOD,
    AI_IMAGE_HTTP_PATH,
    AI_IMAGE_HTTP_PORT,
    AiImageHttpRequestBody,
    AiImageServer as InnerServer,
    aiImageHttpResponseBody,
} from "@core/apps/ai-image";
import { CancelListening, EventBroker } from "@core/event";
import { TrustedService } from "@core/net";

export interface GeneratedImage {
    url: string;
    prompt: string;
}

export class AiImageServer {
    #inner: InnerServer;
    #onImageGenerated = new EventBroker<GeneratedImage>();

    constructor(trustedService: TrustedService) {
        this.#inner = new InnerServer(trustedService);
        this.#inner.onReceive(async (packet) => {
            const param: AiImageHttpRequestBody = { prompt: packet.prompt };
            const url = `${AI_IMAGE_HTTP_PATH}:${AI_IMAGE_HTTP_PORT}`;
            const res = await fetch(url, {
                method: AI_IMAGE_HTTP_METHOD,
                body: JSON.stringify(param),
            })
                .then(async (res) => aiImageHttpResponseBody.parse(await res.json()))
                .catch((e) => ({ error: e }));

            if ("error" in res) {
                console.error("Failed to generate image", res.error);
                return;
            }

            this.#onImageGenerated.emit({
                url: res.imageUrl,
                prompt: param.prompt,
            });
        });
    }

    onImageGenerated(handler: (image: GeneratedImage) => void): CancelListening {
        return this.#onImageGenerated.listen(handler);
    }

    start() {
        return this.#inner.start();
    }
}
