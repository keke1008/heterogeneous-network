import { EchoServer } from "@core/apps/echo";
import { TrustedService } from "@core/net";
import { Err, Ok, Result } from "oxide.ts";
import { AiImageGenerationServer } from "./ai-image";

export class AppServer {
    #trustedService: TrustedService;

    #echo?: EchoServer;
    #aiImageGeneration?: AiImageGenerationServer;

    constructor(args: { trustedService: TrustedService }) {
        this.#trustedService = args.trustedService;
    }

    startEcho(): Result<void, "already opened" | "already started"> {
        if (this.#echo) {
            return Err("already started");
        }

        this.#echo = new EchoServer({ trustedService: this.#trustedService });
        return this.#echo.start();
    }

    stopEcho(): void {
        this.#echo?.close();
        this.#echo = undefined;
    }

    startAiImageGeneration(): Result<void, "already opened" | "already started"> {
        if (this.#aiImageGeneration) {
            return Err("already started");
        }

        this.#aiImageGeneration = new AiImageGenerationServer();
        return Ok(undefined);
    }

    stopAiImageGeneration(): void {
        this.#aiImageGeneration?.close();
        this.#aiImageGeneration = undefined;
    }
}
