import { Err, Result } from "oxide.ts";
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

        return TrustedSocket.activeOpen({
            socket: tunnelSocket.unwrap(),
            localNodeService: this.#localNodeService,
        });
    }

    listen(localPortId: TunnelPortId, callback: (socket: TrustedSocket) => void): Result<() => void, "already opened"> {
        return this.#tunnelService.listen(localPortId, async (tunnelSocket) => {
            const socket = await TrustedSocket.passiveOpen({
                socket: tunnelSocket,
                localNodeService: this.#localNodeService,
            });
            if (socket.isErr()) {
                return;
            }

            callback(socket.unwrap());
        });
    }
}
