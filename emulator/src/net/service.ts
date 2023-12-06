import { Address, AddressType, NetFacade, RpcService, SerialAddress, WebSocketAddress } from "@core/net";
import { Cost, NetworkUpdate } from "@core/net/node";
import { LinkStateService } from "./linkState";
import { PortAlreadyOpenError, SerialHandler } from "./media/serial";
import { WebSocketAlreadyConnectedError, WebSocketHandler } from "./media/websocket";
import { None, Option, Result, Some } from "oxide.ts";
import { CancelListening } from "@core/event";

export interface InitializeParameter {
    localSerialAddress: SerialAddress;
    localCost?: Cost;
}

export class NetService {
    #net: NetFacade;
    #linkState: LinkStateService;
    #serialHandler: Option<SerialHandler> = None;
    #webSocketHandler: Option<WebSocketHandler> = None;

    constructor() {
        this.#net = new NetFacade();
        this.#linkState = new LinkStateService(this.#net);
    }

    registerFrameHandler(params: InitializeParameter): void {
        if (this.#serialHandler.isSome() || this.#webSocketHandler.isSome()) {
            return;
        }

        const serialHandler = new SerialHandler(params.localSerialAddress);
        this.#serialHandler = Some(serialHandler);
        this.#net.addHandler(AddressType.Serial, serialHandler);

        const webSocketHandler = new WebSocketHandler();
        this.#webSocketHandler = Some(webSocketHandler);
        this.#net.addHandler(AddressType.WebSocket, webSocketHandler);
    }

    async connectSerial(args: { remoteAddress: SerialAddress; linkCost: Cost }): Promise<Result<void, unknown>> {
        const result = await this.#serialHandler.unwrap().addPort();
        if (result.isErr() && !(result.unwrapErr() instanceof PortAlreadyOpenError)) {
            return result;
        }
        return this.#net.neighbor().sendHello(new Address(args.remoteAddress), args.linkCost);
    }

    async connectWebSocket(args: { remoteAddress: WebSocketAddress; linkCost: Cost }): Promise<Result<void, unknown>> {
        const result = await this.#webSocketHandler.unwrap().connect(args.remoteAddress);
        if (result.isErr() && !(result.unwrapErr() instanceof WebSocketAlreadyConnectedError)) {
            return result;
        }
        return this.#net.neighbor().sendHello(new Address(args.remoteAddress), args.linkCost);
    }

    dumpNetworkStateAsUpdate(): NetworkUpdate[] {
        return this.#linkState.dumpNetworkStateAsUpdate(this.#net);
    }

    onNetworkUpdate(onStateUpdate: (updates: NetworkUpdate[]) => void): CancelListening {
        return this.#linkState.onNetworkUpdate(onStateUpdate);
    }

    end(): void {
        this.#net.dispose();
    }

    rpc(): RpcService {
        return this.#net.rpc();
    }
}
