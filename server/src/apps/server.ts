import { EchoServer } from "@core/apps/echo";
import { CaptionServer } from "@core/apps/caption";
import { TrustedService } from "@core/net";
import { Err, Result } from "oxide.ts";

export class AppServer {
    #trustedService: TrustedService;

    #echo?: EchoServer;
    #caption?: CaptionServer;

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

    startCaption(): Result<void, "already opened" | "already started"> {
        if (this.#caption) {
            return Err("already started");
        }

        this.#caption = new CaptionServer(this.#trustedService, () => {});
        return this.#caption.start();
    }

    stopEcho(): void {
        this.#echo?.close();
        this.#echo = undefined;
    }
}
