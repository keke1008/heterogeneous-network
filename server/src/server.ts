import { AddressType, NetFacadeBuilder, NodeId, Procedure, UdpAddress } from "@core/net";
import { UdpHandler, getLocalIpV4Addresses } from "@core/media/dgram";
import { WebSocketHandler } from "./websocket";
import * as Rpc from "./rpc";
import { VRouterService } from "./vrouter";

const UDP_SERVER_LISTEN_PORT = 12345;
const WEBSOCKET_SERVER_LISTEN_PORT = 12346;

const main = async (): Promise<void> => {
    console.log("Initializing...");
    const net = new NetFacadeBuilder().buildWithDefaults();

    const udpHandler = new UdpHandler(UDP_SERVER_LISTEN_PORT);
    net.addHandler(AddressType.Udp, udpHandler);

    const webSocketHandler = new WebSocketHandler({ port: WEBSOCKET_SERVER_LISTEN_PORT });
    net.addHandler(AddressType.WebSocket, webSocketHandler);

    const ipAddr = getLocalIpV4Addresses()[0];
    const localAddress = UdpAddress.fromHumanReadableString(ipAddr, UDP_SERVER_LISTEN_PORT).expect("Invalid address");
    net.localNode().tryInitialize(NodeId.fromAddress(localAddress));

    const vRouterService = new VRouterService();
    const rpc = net.rpc();
    rpc.addServer(Procedure.GetVRouters, new Rpc.GetVRoutersServer({ vRouterService }));
    rpc.addServer(Procedure.CreateVRouter, new Rpc.CreateVRouterServer({ vRouterService }));
    rpc.addServer(Procedure.DeleteVRouter, new Rpc.DeleteVRouterServer({ vRouterService }));
};

main();
