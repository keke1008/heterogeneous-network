import { CaptionServer } from "@core/apps/caption";
import { TrustedService } from "@core/net";
import { createCaptionServer } from "./caption";
import { Err, Result } from "oxide.ts/core";
import { EchoServer } from "@core/apps/echo";

export class AppServer {
    #trustedService: TrustedService;
    #echo?: EchoServer;
    #caption?: CaptionServer;

    constructor(args: { trustedService: TrustedService }) {
        this.#trustedService = args.trustedService;
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
}
