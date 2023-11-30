import { AddressType, Cost, NetFacade, NodeId, RpcService, SerialAddress, WebSocketAddress } from "@core/net";
import { LinkStateService, StateUpdate } from "./linkState";
import { SerialHandler } from "./media/serial";
import { WebSocketHandler } from "./media/websocket";
import { Result } from "oxide.ts";

export class NetService {
    #net: NetFacade;
    #linkState: LinkStateService;
    #serialHandler: SerialHandler;
    #webSocketHandler: WebSocketHandler;

    constructor(args: { localSerialAddress: SerialAddress; localCost?: Cost }) {
        const localId = NodeId.fromAddress(args.localSerialAddress);
        this.#net = new NetFacade(localId, args.localCost ?? new Cost(0));

        this.#linkState = new LinkStateService(this.#net);

        this.#serialHandler = new SerialHandler(args.localSerialAddress);
        this.#net.addHandler(AddressType.Serial, this.#serialHandler);

        this.#webSocketHandler = new WebSocketHandler();
        this.#net.addHandler(AddressType.WebSocket, this.#webSocketHandler);
    }

    connectSerial(): Promise<Result<void, unknown>> {
        return this.#serialHandler.addPort();
    }

    connectWebSocket(address: WebSocketAddress): Promise<Result<void, unknown>> {
        return this.#webSocketHandler.connect(address);
    }

    syncNetState(): StateUpdate {
        return this.#linkState.syncState();
    }

    onNetStateUpdate(onStateUpdate: (update: StateUpdate) => void): void {
        this.#linkState.onStateUpdate(onStateUpdate);
    }

    end(): void {
        this.#net.dispose();
    }

    rpc(): RpcService {
        return this.#net.rpc();
    }
}
