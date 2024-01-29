import { EchoServer } from "@core/apps/echo";
import { TrustedService } from "@core/net";
import { Err, Result } from "oxide.ts";

export class AppServer {
    #trustedService: TrustedService;

    #echo?: EchoServer;

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
}
