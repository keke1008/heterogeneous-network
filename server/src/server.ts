import { AddressType, NetFacadeBuilder } from "@core/net";
import { UdpHandler } from "@core/media/dgram";
import { WebSocketHandler } from "./websocket";

const UDP_SERVER_LISTEN_PORT = 12345;
const WEBSOCKET_SERVER_LISTEN_PORT = 12346;

const main = async (): Promise<void> => {
    console.log("Initializing...");
    const net = new NetFacadeBuilder().buildWithDefaults();

    const udpHandler = new UdpHandler(UDP_SERVER_LISTEN_PORT);
    net.addHandler(AddressType.Udp, udpHandler);

    const webSocketHandler = new WebSocketHandler({ port: WEBSOCKET_SERVER_LISTEN_PORT });
    net.addHandler(AddressType.WebSocket, webSocketHandler);
};

main();
