import { AddressType, NetFacade, NetFacadeBuilder, NodeId, Procedure, UdpAddress } from "@core/net";
import { UdpHandler, getLocalIpV4Addresses } from "@core/media/dgram";
import { WebSocketHandler } from "./websocket";
import * as Rpc from "./rpc";
import { VRouterService } from "@vrouter/service";
import { AppServer } from "./apps/server";

const UDP_SERVER_LISTEN_PORT = 12345;
const WEBSOCKET_SERVER_LISTEN_PORT = 12346;

const createNetFacade = async (): Promise<NetFacade> => {
    console.log("Initializing...");
    const net = new NetFacadeBuilder().buildWithDefaults();

    const udpHandler = new UdpHandler(UDP_SERVER_LISTEN_PORT);
    net.addHandler(AddressType.Udp, udpHandler);

    const webSocketHandler = new WebSocketHandler({ port: WEBSOCKET_SERVER_LISTEN_PORT });
    net.addHandler(AddressType.WebSocket, webSocketHandler);

    const ipAddr = getLocalIpV4Addresses()[0];
    const localAddress = UdpAddress.schema.parse([ipAddr, UDP_SERVER_LISTEN_PORT]);
    net.localNode().tryInitialize(NodeId.fromAddress(localAddress));

    const vRouterService = new VRouterService();
    const rpc = net.rpc();
    rpc.addServer(Procedure.GetVRouters, new Rpc.GetVRoutersServer({ vRouterService }));
    rpc.addServer(Procedure.CreateVRouter, new Rpc.CreateVRouterServer({ vRouterService }));
    rpc.addServer(Procedure.DeleteVRouter, new Rpc.DeleteVRouterServer({ vRouterService }));

    net.observer().launchSinkService();

    return net;
};

const createAppServer = async (net: NetFacade) => {
    const appServer = new AppServer({ trustedService: net.trusted() });
    appServer.startEcho().unwrap();
    appServer.startAiImageGeneration().unwrap();

    return appServer;
};

const main = async () => {
    const net = await createNetFacade();
    await createAppServer(net);
    console.log("Ready");
};

main();
