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

    registerSerialHandler(address: SerialAddress): void {
        if (this.#serialHandler.isSome()) {
            throw new Error("Serial handler is already registered");
        }

        const handler = new SerialHandler(address);
        this.#serialHandler = Some(handler);
        this.#net.addHandler(AddressType.Serial, handler);
    }

    registerWebSocketHandler(): void {
        if (this.#webSocketHandler.isSome()) {
            throw new Error("WebSocket handler is already registered");
        }

        const handler = new WebSocketHandler();
        this.#webSocketHandler = Some(handler);
        this.#net.addHandler(AddressType.WebSocket, handler);
    }

    async connectSerial(args: { remoteAddress: SerialAddress; linkCost: Cost }): Promise<Result<void, unknown>> {
        const result = await this.#serialHandler.unwrap().addPort();
        if (result.isErr() && !(result.unwrapErr() instanceof PortAlreadyOpenError)) {
            return result;
        }
        return this.#net.routing().requestHello(new Address(args.remoteAddress), args.linkCost);
    }

    async connectWebSocket(args: { remoteAddress: WebSocketAddress; linkCost: Cost }): Promise<Result<void, unknown>> {
        const result = await this.#webSocketHandler.unwrap().connect(args.remoteAddress);
        if (result.isErr() && !(result.unwrapErr() instanceof WebSocketAlreadyConnectedError)) {
            return result;
        }
        return this.#net.routing().requestHello(new Address(args.remoteAddress), args.linkCost);
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
