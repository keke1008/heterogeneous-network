import { Address, AddressType, Cost, NetFacade, NodeId, RpcService, SerialAddress, WebSocketAddress } from "@core/net";
import { LinkStateService, StateUpdate } from "./linkState";
import { SerialHandler } from "./media/serial";
import { WebSocketHandler } from "./media/websocket";
import { Result } from "oxide.ts";
import { CancelListening } from "@core/event";

export interface InitializeParameter {
    localSerialAddress: SerialAddress;
    localCost?: Cost;
}

export class NetService {
    #net: NetFacade;
    #linkState: LinkStateService;
    #serialHandler: SerialHandler;
    #webSocketHandler: WebSocketHandler;

    constructor(args: InitializeParameter) {
        const localId = NodeId.fromAddress(args.localSerialAddress);
        this.#net = new NetFacade(localId, args.localCost ?? new Cost(0));

        this.#linkState = new LinkStateService(this.#net);

        this.#serialHandler = new SerialHandler(args.localSerialAddress);
        this.#net.addHandler(AddressType.Serial, this.#serialHandler);

        this.#webSocketHandler = new WebSocketHandler();
        this.#net.addHandler(AddressType.WebSocket, this.#webSocketHandler);
    }

    async connectSerial(args: { remoteAddress: SerialAddress; linkCost: Cost }): Promise<Result<void, unknown>> {
        const result = await this.#serialHandler.addPort();
        return result.andThen(() => this.#net.routing().requestHello(new Address(args.remoteAddress), args.linkCost));
    }

    async connectWebSocket(args: { remoteAddress: WebSocketAddress; linkCost: Cost }): Promise<Result<void, unknown>> {
        const result = await this.#webSocketHandler.connect(args.remoteAddress);
        return result.andThen(() => this.#net.routing().requestHello(new Address(args.remoteAddress), args.linkCost));
    }

    syncNetState(): StateUpdate {
        return this.#linkState.syncState();
    }

    onNetStateUpdate(onStateUpdate: (update: StateUpdate) => void): CancelListening {
        return this.#linkState.onStateUpdate(onStateUpdate);
    }

    end(): void {
        this.#net.dispose();
    }

    rpc(): RpcService {
        return this.#net.rpc();
    }
}
