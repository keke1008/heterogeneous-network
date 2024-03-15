import { CaptionServer } from "@core/apps/caption";
import { StreamService, TrustedService, TunnelService } from "@core/net";
import { createCaptionServer } from "./caption";
import { Err, Ok, Result } from "oxide.ts/core";
import { EchoServer } from "@core/apps/echo";
import { FileServer } from "@core/apps/file";
import { ChatApp } from "@core/apps/chat";
import { AiImageServer } from "./aiImage";
import { PostingServer } from "@core/apps/posting";

export class AppServer {
    #tunnelService: TunnelService;
    #trustedService: TrustedService;
    #streamService: StreamService;

    #echo?: EchoServer;
    #caption?: CaptionServer;
    #file?: FileServer;
    #aiImage?: AiImageServer;
    #chatApp?: ChatApp;
    #posting?: PostingServer;

    constructor(args: { tunnelService: TunnelService; trustedService: TrustedService; streamService: StreamService }) {
        this.#tunnelService = args.tunnelService;
        this.#trustedService = args.trustedService;
        this.#streamService = args.streamService;
    }

    startEchoServer(): Result<void, "already opened" | "already started"> {
        if (this.#echo) {
            return Err("already opened");
        }
        this.#echo = new EchoServer({ trustedService: this.#trustedService });
        return this.#echo.start();
    }

    startCaptionServer(): Result<void, "already opened" | "already started"> {
        if (this.#caption) {
            return Err("already started");
        }
        this.#caption = createCaptionServer(this.#trustedService);
        return this.#caption.start();
    }

    startFileServer(): Result<void, "already opened" | "already started"> {
        if (this.#file) {
            return Err("already started");
        }
        this.#file = new FileServer({ service: this.#streamService });
        return this.#file.start();
    }

    startAiImageServer(): Result<void, "already opened" | "already started"> {
        if (this.#aiImage) {
            return Err("already started");
        }
        this.#aiImage = new AiImageServer(this.#trustedService);
        return this.#aiImage.start();
    }

    startChatApp(): Result<void, "already opened" | "already started"> {
        if (this.#chatApp) {
            return Err("already started");
        }
        this.#chatApp = new ChatApp(this.#streamService);
        return Ok(undefined);
    }

    startPostingServer(): Result<void, "already opened" | "already started"> {
        if (this.#posting) {
            return Err("already started");
        }
        this.#posting = new PostingServer({ tunnelServer: this.#tunnelService });
        return this.#posting.start();
    }

    aiImageServer(): AiImageServer {
        if (!this.#aiImage) {
            throw new Error("AiImageServer not started");
        }
        return this.#aiImage;
    }

    chatApp(): ChatApp {
        if (!this.#chatApp) {
            throw new Error("ChatApp not started");
        }
        return this.#chatApp;
    }

    postingServer(): PostingServer {
        if (!this.#posting) {
            throw new Error("PostingServer not started");
        }
        return this.#posting;
    }
}
