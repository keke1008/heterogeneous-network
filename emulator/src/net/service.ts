import {
    Address,
    AddressType,
    NetFacade,
    NetFacadeBuilder,
    RpcService,
    SerialAddress,
    StreamService,
    TrustedService,
    WebSocketAddress,
} from "@core/net";
import { Cost, NetworkTopologyUpdate } from "@core/net/node";
import { LinkStateService } from "./linkState";
import { PortAlreadyOpenError, SerialHandler } from "@media/serial-browser";
import { WebSocketHandler } from "@media/websocket-browser";
import { Err, None, Ok, Option, Result, Some } from "oxide.ts";
import { CancelListening } from "@core/event";
import { LocalNodeService } from "@core/net/local";
import { NetworkUpdate } from "@core/net/observer/frame";

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
        this.#net = new NetFacadeBuilder().buildWithDefaults();
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

    async connectWebSocket(args: { remoteAddress: WebSocketAddress; linkCost: Cost }): Promise<Result<void, string>> {
        const connectResult = await this.#webSocketHandler.unwrap().connect(args.remoteAddress);
        if (connectResult.isErr() && !(connectResult.unwrapErr() === "already connected")) {
            return connectResult;
        }
        const sendHelloResult = await this.#net.neighbor().sendHello(new Address(args.remoteAddress), args.linkCost);
        if (sendHelloResult.isErr()) {
            return Err(sendHelloResult.unwrapErr().type);
        }

        return Ok(undefined);
    }

    dumpNetworkStateAsUpdate(): NetworkTopologyUpdate[] {
        return this.#linkState.dumpNetworkStateAsUpdate(this.#net);
    }

    onNetworkTopologyUpdate(callback: (updates: NetworkUpdate[]) => void): CancelListening {
        return this.#linkState.onNetworkUpdate(callback);
    }

    end(): void {
        this.#net.dispose();
    }

    rpc(): RpcService {
        return this.#net.rpc();
    }

    localNode(): LocalNodeService {
        return this.#net.localNode();
    }

    trusted(): TrustedService {
        return this.#net.trusted();
    }

    stream(): StreamService {
        return this.#net.stream();
    }
}
