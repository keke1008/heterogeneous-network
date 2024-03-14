import { AddressType, NetFacade, NetFacadeBuilder } from "@core/net";
import { UdpHandler, getLocalIpV4Addresses } from "@core/media/dgram";
import { WebSocketHandler } from "@core/media/websocket";

const UDP_PORT = 12345;

export class NetCore {
    #net: NetFacade;

    constructor() {
        this.#net = new NetFacadeBuilder().buildWithDefaults();
        this.#net.addHandler(AddressType.Udp, new UdpHandler(12345));
        this.#net.addHandler(AddressType.WebSocket, new Websockethandler());
    }
}
