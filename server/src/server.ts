import { AddressType, NetFacadeBuilder, Procedure } from "@core/net";
import { UdpHandler } from "@core/media/dgram";
import { WebSocketHandler } from "./websocket";
import * as Rpc from "./rpc";
import { VRouterService } from "./vrouter";
import { IpAddressWithPrefix } from "./command/ip";

const UDP_SERVER_LISTEN_PORT = 12345;
const WEBSOCKET_SERVER_LISTEN_PORT = 12346;
const ROOT_BRIDGE_LOCAL_ADDRESS = IpAddressWithPrefix.schema.parse("10.0.0.254/24");
const VROUTER_LOCAL_ADDRESS_RANGE = IpAddressWithPrefix.schema.parse("10.0.0.0/24");

const main = async (): Promise<void> => {
    console.log("Initializing...");
    const net = new NetFacadeBuilder().buildWithDefaults();

    const udpHandler = new UdpHandler(UDP_SERVER_LISTEN_PORT);
    net.addHandler(AddressType.Udp, udpHandler);

    const webSocketHandler = new WebSocketHandler({ port: WEBSOCKET_SERVER_LISTEN_PORT });
    net.addHandler(AddressType.WebSocket, webSocketHandler);

    const vRouterService = await VRouterService.create({
        rootBridgeLocalAddress: ROOT_BRIDGE_LOCAL_ADDRESS,
        vRouterLocalAddreessRange: VROUTER_LOCAL_ADDRESS_RANGE,
    });
    const rpc = net.rpc();
    rpc.addServer(Procedure.GetVRouters, new Rpc.GetVRoutersServer({ vRouterService }));
    rpc.addServer(Procedure.CreateVRouter, new Rpc.CreateVRouterServer({ vRouterService }));
    rpc.addServer(Procedure.DeleteVRouter, new Rpc.DeleteVRouterServer({ vRouterService }));
};

main();
