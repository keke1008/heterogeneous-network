import { AiImageServer as InnerServer, fetchAiImageUrl } from "@core/apps/ai-image";
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
            const res = await fetchAiImageUrl(packet.httpServerAddress, packet.prompt);
            if (res.isErr()) {
                console.error("Failed to generate image", res.unwrapErr());
                return;
            }

            this.#onImageGenerated.emit({
                url: res.unwrap(),
                prompt: packet.prompt,
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
