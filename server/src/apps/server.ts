import { EchoServer } from "@core/apps/echo";
import { PostingServer } from "@core/apps/posting";
import { TrustedService, TunnelService } from "@core/net";
import { Err, Ok, Result } from "oxide.ts";
import { AiImageGenerationServer } from "./ai-image";

export class AppServer {
    #tunnelService: TunnelService;
    #trustedService: TrustedService;

    #echo?: EchoServer;
    #aiImageGeneration?: AiImageGenerationServer;
    #posting?: PostingServer;

    constructor(args: { tunnelService: TunnelService; trustedService: TrustedService }) {
        this.#tunnelService = args.tunnelService;
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

    startPosting(): Result<void, "already opened" | "already started"> {
        if (this.#posting) {
            return Err("already started");
        }

        this.#posting = new PostingServer({ tunnelServer: this.#tunnelService });
        this.#posting.onReceive((data) => {
            console.log("PostingServer received: ", data);
        });
        return this.#posting.start();
    }

    stopPosting(): void {
        this.#posting?.close();
        this.#posting = undefined;
    }
}
