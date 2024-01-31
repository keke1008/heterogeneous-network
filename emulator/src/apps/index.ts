import { CaptionServer } from "@core/apps/caption";
import { StreamService, TrustedService } from "@core/net";
import { createCaptionServer } from "./caption";
import { Err, Result } from "oxide.ts/core";
import { EchoServer } from "@core/apps/echo";
import { FileServer } from "@core/apps/file";

export class AppServer {
    #trustedService: TrustedService;
    #streamService: StreamService;

    #echo?: EchoServer;
    #caption?: CaptionServer;
    #file?: FileServer;

    constructor(args: { trustedService: TrustedService; streamService: StreamService }) {
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
}
