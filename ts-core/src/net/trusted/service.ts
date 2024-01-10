import { Err, Ok, Result } from "oxide.ts";
import { Destination } from "../node";
import { TunnelPortId, TunnelService } from "../tunnel";
import { TrustedSocket } from "./socket";
import { LocalNodeService } from "../local";

export class TrustedService {
    #localNodeService: LocalNodeService;
    #tunnelService: TunnelService;

    constructor(args: { localNodeService: LocalNodeService; tunnelService: TunnelService }) {
        this.#localNodeService = args.localNodeService;
        this.#tunnelService = args.tunnelService;
    }

    async connect(args: {
        localPortId?: TunnelPortId;
        destination: Destination;
        destinationPortId: TunnelPortId;
    }): Promise<Result<TrustedSocket, "timeout" | "already opened">> {
        let tunnelSocket;
        if (args.localPortId) {
            tunnelSocket = this.#tunnelService.open({
                localPortId: args.localPortId,
                destination: args.destination,
                destinationPortId: args.destinationPortId,
            });
        } else {
            tunnelSocket = this.#tunnelService.openDynamicPort({
                destination: args.destination,
                destinationPortId: args.destinationPortId,
            });
        }

        if (tunnelSocket.isErr()) {
            return Err(tunnelSocket.unwrapErr());
        }

        const socket = await TrustedSocket.connect(tunnelSocket.unwrap(), this.#localNodeService);
        if (socket.isErr()) {
            const message = socket.unwrapErr();
            if (message === "invalid operation") {
                throw new Error("Unreachable");
            } else {
                return Err(message);
            }
        }

        return Ok(socket.unwrap());
    }

    listen(localPortId: TunnelPortId, callback: (socket: TrustedSocket) => void): Result<() => void, "already opened"> {
        return this.#tunnelService.listen(localPortId, async (tunnelSocket) => {
            const socket = await TrustedSocket.accept(tunnelSocket, this.#localNodeService);
            if (socket.isOk()) {
                callback(socket.unwrap());
            } else {
                console.warn("failed to accept socket", socket.unwrapErr());
            }
        });
    }
}
