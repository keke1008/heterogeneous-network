import { Ok, Result } from "oxide.ts";
import { Destination } from "../node";
import { TrustedService } from "../trusted";
import { TunnelPortId } from "../tunnel";
import { StreamSocket } from "./socket";

export class StreamService {
    #trustedService: TrustedService;

    constructor(args: { trustedService: TrustedService }) {
        this.#trustedService = args.trustedService;
    }

    async connect(args: {
        localPortId?: TunnelPortId;
        destination: Destination;
        destinationPortId: TunnelPortId;
    }): Promise<Result<StreamSocket, "timeout" | "already opened">> {
        const trustedSocketResult = await this.#trustedService.connect({
            localPortId: args.localPortId,
            destination: args.destination,
            destinationPortId: args.destinationPortId,
        });
        if (trustedSocketResult.isErr()) {
            return trustedSocketResult;
        }

        return Ok(new StreamSocket(trustedSocketResult.unwrap()));
    }

    listen(localPortId: TunnelPortId, callback: (socket: StreamSocket) => void) {
        return this.#trustedService.listen(localPortId, (trustedSocket) => {
            callback(new StreamSocket(trustedSocket));
        });
    }
}
